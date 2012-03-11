<?php
/**
 * @file
 * Functions related to importing and publishing patterns on the web.
 */

/**
 * Form constructor for the pattern importing from server form.
 *
 * @see patterns_import_server_validate()
 * @see patterns_import_server_submit()
 * @ingroup forms
 */
function patterns_import_server($form, &$form_state) {
  if (empty($form_state['storage'])) {

    // real_path(public) returns the path to the files dir.
    $path = drupal_realpath('public://') . "/" . variable_get('patterns_save_file', 'patterns');

    // Ste: was
    //if (!file_check_directory($path, true)) {
    if (!file_prepare_directory($path)) {
      $message = t("In order to save imported patterns, directory %path must be writable.<br />", array('%path' => $path));
      $link = l(t('status report'), 'admin/reports/status');
      $message .= t('You might want to check the !link to ensure your files directory exists and is writable.', array('!link' => $link));
      drupal_set_message($message, 'error');
      return array();
    }

    $form['server_url'] = array(
      '#type' => 'textfield',
      '#title' => t('Specify Patterns Server URL'),
      '#description' => t('URL of the web site you want to import patterns from. Example: http://patterns.graviteklabs.com'),
      '#default_value' => variable_get('patterns_default_server', 'http://patterns.graviteklabs.com'),
      '#size' => 48
    );
  }
  else {
    $feed = $form_state['storage']['feed'];

    $form['patterns'] = array(
      '#type' => 'fieldset',
      '#tree' => TRUE,
      '#title' => t('Patterns'),
    );

    foreach ($feed['patterns'] as $pattern) {
      $form['patterns'][$pattern['pid']] = array(
        '#type' => 'checkbox',
        '#title' => $pattern['title'],
        '#description' => $pattern['description'],
        '#disabled' => in_array($pattern['file_type'], patterns_config_get_file_types()) ? FALSE : TRUE,
      );
    }
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => empty($form_state['storage']) ? t('Get Patterns List') : t('Import')
  );

  $form['#validate'][] = 'patterns_import_server_validate';
  $form['#submit'][] = 'patterns_import_server_submit';

  return $form;
}

/**
 * Form submission handler for patterns_import_server().
 *
 * @see patterns_import_server_validate()
 */
function patterns_import_server_validate($form, &$form_state) {
  if (empty($form_state['storage'])) {
    if (!ini_get('allow_url_fopen')) {
      form_set_error('fopen', t('allow_url_fopen must be enabled in your php configuration in order to use this feature.'));
      return;
    }
    $feed_url = $form_state['values']['server_url'] . '/patterns.xml';
    if (empty($feed_url)) {
      form_set_error('server_url', t('Please enter Patterns Server URL.'));
      return;
    }
    if (!$xml = file_get_contents($feed_url)) {
      form_set_error('server_url', t('Failed to retreive the patterns feed from: %url. Please check your URL and try again.', array('%url' => $form_state['values']['server_url'])));
      return;
    }
    // @todo: replace this with proper XML validation.
    if (strpos($xml, '<?xml') !== 0) {
      form_set_error('', t('URL %url is not a valid XML file.', array('%url' => $form_state['values']['server_url'])));
      return;
    }

    $feed = patterns_from_source($xml);
    if (!$feed) {
      form_set_error('', t('URL %url is not a valid patterns feed.', array('%url' => $form_state['values']['server_url'])));
      return;
    }
    $feed = patterns_feed_rearrange_data($feed);
    if (!$feed) {
      form_set_error('', t('URL %url is not a valid patterns feed.', array('%url' => $form_state['values']['server_url'])));
      return;
    }

    $form_state['storage']['step'] = 'get url';
    $form_state['storage']['server_url'] = $form_state['values']['server_url'];
    $form_state['storage']['feed_url'] = $feed_url;
    $form_state['storage']['feed'] = $feed;
  }
  else {
    $form_state['storage']['step'] = 'select patterns';
  }
}

/**
 * Form validation handler for patterns_import_server().
 *
 * @see patterns_import_server_submit()
 */
function patterns_import_server_submit($form, &$form_state) {

  if ($form_state['storage']['step'] == 'get url') {
    return;
  }

  $pids = array_keys(array_filter($form_state['values']['patterns']));
  $feed = $form_state['storage']['feed'];
  $errors = array();

  foreach ($pids as $pid) {
    $url = $feed['info']['url'] . '/admin/patterns/get/' . $pid;
    $pattern_info = $feed['patterns'][$pid];

    if (!$source = file_get_contents($url)) {
      $errors['get file'][] = $pattern_info;
      continue;
    }

    // real_path(public) returns the path to the files dir
    $path = drupal_realpath('public://') . "/" . variable_get('patterns_save_file', 'patterns');
    $path .= '/' . $pattern_info['name'] . '.' . $pattern_info['file_type'];
    if (!$saved = file_save_data($source, $path, FILE_EXISTS_ERROR)) {
      $errors['save file'][] = $pattern_info;
      continue;
    }

    // Choose appropriate function based on the file extension.
    $func = 'patterns_load_' . $pattern_info['file_type'];

    // Load pattern.
    if (!$pattern = $func($saved)) {
      $errors['load pattern'][] = $pattern_info;
      unlink($saved);
      continue;
    }

    patterns_save_pattern($pattern, $saved, $pattern_info['name']);
  }

  if (!empty($errors)) {
    $patterns = array();
    foreach ($errors as $type => $files) {
      foreach ($files as $file) {
        $patterns[] = $file['title'] . ' (cause: "' . $type . '" failed)'; // TODO: t()
      }
      $patterns = theme('item_list', array('items' => $patterns)); // TODO: ???
    }
    drupal_set_message(t('Import failed for the following patterns:') . '<br/>' . $patterns, 'error');
    return;
  }

  unset($form_state['storage']);
  $form_state['redirect'] = 'admin/patterns';

}

function patterns_feed_rearrange_data($feed) {
  unset($feed['tag']);
  foreach ($feed as $key => $section) {
    $tag = $section['tag'];
    if ($tag == 'pattern') {
      unset($section['tag']);
      if (!isset($section['value'])) {
        foreach ($section as $t) {
          if ($t['tag'] == 'pid') {
            $pid = $t['value'];
            break;
          }
        }
        $result[$pid] = patterns_feed_rearrange_data($section);
      }
      else {
        $result[$tag] = $section['value'];
      }
    }
    else {
      unset($section['tag']);
      if (!isset($section['value'])) {
        $result[$tag] = patterns_feed_rearrange_data($section);
      }
      else {
        $result[$tag] = $section['value'];
      }
    }
  }
  return $result;
}


/**
 * @TODO Not really doing anything for now.
 * @param mixed $pid
 */
function patterns_publish_pattern($pid) {
  if (is_numeric($pid)) {
    $result = db_query("UPDATE {patterns} SET public = 1 WHERE pid = :pid", array('pid' => $pid));
  }
  drupal_set_message(t('Pattern published. Only in the db.'));
  drupal_goto('admin/patterns');
}

/**
 * @TODO Not really doing anything for now.
 * @param mixed $pid
 */
function patterns_unpublish_pattern($pid) {
  if (is_numeric($pid)) {
    $result = db_query("UPDATE {patterns} SET public = 0 WHERE pid = :pid", array('pid' => $pid));
  }
  drupal_set_message(t('Pattern unpublished.'));
  drupal_goto('admin/patterns');
}
