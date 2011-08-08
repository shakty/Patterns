<?php

/**
 * Display the import pattern from server form
 */
function patterns_import_server($form, &$form_state) {
  if (empty($form_state['storage'])) {

    // Ste: was
    //$path = file_create_path(variable_get('patterns_save_xml', 'patterns'));
    
    // real_path(public) returns the path to the files dir
    $path = drupal_realpath('public://') . "/" . variable_get('patterns_save_xml', 'patterns');
  
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
        '#disabled' => in_array($pattern['file_type'], patterns_file_types()) ? FALSE : TRUE,
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
    // @todo: replace this with proper XML validation
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

    // save file
    // Ste: was
    //$path = file_create_path(variable_get('patterns_save_xml', 'patterns'));
    
    // real_path(public) returns the path to the files dir
    $path = drupal_realpath('public://') . "/" . variable_get('patterns_save_xml', 'patterns');
    $path .= '/' . $pattern_info['name'] . '.' . $pattern_info['file_type'];
    if (!$saved = file_save_data($source, $path, FILE_EXISTS_ERROR)) {
      $errors['save file'][] = $pattern_info;
      continue;
    }

      // choose appropriate function based on the file extension
    $func = 'patterns_load_' . $pattern_info['file_type'];

    // Load pattern
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
        $patterns[] = $file['title'] . ' (cause: "' . $type . '" failed)';
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
 * Not really doing anything for now
 * @param mixed $pid
 */
function patterns_publish_pattern($pid) {
  if (is_numeric($pid)) {
    $result = db_query("UPDATE {patterns} SET public = 1 WHERE pid = :pid", array('pid' => $pid));
  }
  drupal_set_message(t('Pattern published. Only in the db.'));
  drupal_goto('admin/patterns');
}

function patterns_unpublish_pattern($pid) {
  if (is_numeric($pid)) {
    $result = db_query("UPDATE {patterns} SET public = 0 WHERE pid = :pid", array('pid' => $pid));
  }
  drupal_set_message(t('Pattern unpublished.'));
  drupal_goto('admin/patterns');
}

/**
 * Display the import pattern file form
 */
function patterns_import_file(&$form_state) {
  if (empty($form_state['post'])) {
    drupal_set_message(t('Import feature currently supports only XML file format.'), 'warning');
  }
  $form['#attributes']['enctype'] = 'multipart/form-data';
  $form['xmlfile'] = array(
    '#type' => 'file',
    '#title' => t('Upload Pattern File'),
    '#description' => t('Imported patterns are not executed until you run them manually.'),
    '#size' => 48
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  $form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}

/**
 * Display the import pattern url form
 */
function patterns_import_url(&$form_state) {
  if (empty($form_state['post'])) {
    drupal_set_message(t('Import feature currently supports only XML file format.'), 'warning');
  }
  $form['xmlurl'] = array(
    '#type' => 'textfield',
    '#title' => t('Specify an URL'),
    '#description' => t('Import a pattern from a remote URL. Imported patterns are not executed until you run them manually.'),
    '#size' => 48
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  $form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}


/**
 * Display the import pattern form
 */
function patterns_import_source($form, &$form_state) {
  if (empty($form_state['post'])) {
    drupal_set_message(t('Import feature currently supports only XML file format.'), 'warning');
  }
  $form['xmlname'] = array(
    '#type' => 'textfield',
    '#title' => t('Pattern Identifier'),
    '#description' => t('Machine readable name for the pattern. The actual title should be included in the pattern itself.'),
    '#required' => true
  );
  $form['xmlsource'] = array(
    '#type' => 'textarea',
    '#rows' => 15,
    '#title' => t('Enter Pattern Source Code'),
    '#description' => t('Imported patterns are not executed until you run them manually.')
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  $form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}

function patterns_import_validate($form, &$form_state) {

  // Ste: was
    //$path = file_create_path(variable_get('patterns_save_xml', 'patterns'));
    
    // real_path(public) returns the path to the files dir
    $path = drupal_realpath('public://') . "/" . variable_get('patterns_save_xml', 'patterns');
    
  if (!file_prepare_directory($path)) {
    $message = t("Destination folder doesn't exist: !path<br />", array('!path' => $path));
    $link = l(t('status report'), 'admin/reports/status');
    $message .= t('You might want to check the !link to ensure your files directory exists and is writable.', array('!link' => $link));
    form_set_error('xmlfile', $message);
    return;
  }
  $validators = array('file_validate_extensions' => array('xml'));
  if (isset($form_state['values']['xmlfile']) && ($file = file_save_upload('xmlfile', $validators))) {
    $form_state['values']['xmlsource'] = file_get_contents($file->filepath);
    $form_state['pattern_file'] = $file;
    $form_state['pattern_file']->destination = $path;
  }
  elseif (isset($form_state['values']['xmlfile'])) {
    form_set_error('files[xmlfile]', t('Error uploading XML file.'));
    return;
  }
  elseif ($form_state['values']['xmlurl']) {
    if (!ini_get('allow_url_fopen')) {
      form_set_error('xmlsource', t('allow_url_fopen must be enabled in your php configuration in order to use this feature.'));
      return;
    }

    if (strtolower(substr($form_state['values']['xmlurl'], -4)) != '.xml') {
      form_set_error('xmlsource', t('Invalid file extension. Only "XML" file extension is allowed.'));
      return;
    }

    if (!($form_state['values']['xmlsource'] = file_get_contents($form_state['values']['xmlurl']))) {
      form_set_error('xmlurl', t('Failed to retreive the pattern specified: %xmlurl. Check your URL and try again.', array('%xmlurl' => $form_state['values']['xmlurl'])));
      return;
    }

    $pattern = array('/\.[^\.]*$/', '/[^a-zA-Z0-9_]/');
    $replacement = array('', '_');
    $form_state['values']['xmlname'] = preg_replace($pattern, $replacement, basename($form_state['values']['xmlurl']));
  }

  if (strpos($form_state['values']['xmlsource'], '<?xml') !== 0) {
    $form_state['values']['xmlsource'] = '<?xml version="1.0" encoding="ISO-8859-1"?>' . $form_state['values']['xmlsource'];
  }

  if ($form_state['values']['xmlname'] && preg_match('/[^a-zA-Z0-9_]/', $form_state['values']['xmlname'])) {
    form_set_error('xmlname', t('You can only include letters, numbers, and underscores in the pattern identifier.'));
  }
  elseif ($form_state['values']['xmlname'] && preg_match('/^_/', $form_state['values']['xmlname'])) {
    form_set_error('xmlname', t('You cannot start the pattern identifier with an underscore.'));
  }

  // @TODO validate XML and don't allow import if validation fails
  $parse = drupal_xml_parser_create($form_state['values']['xmlsource']);
  $success = xml_parse_into_struct($parse, $form_state['values']['xmlsource'], $vals, $index);

  // Check that the xml was properly parsed and also that the
  // root <pattern> tag and also an <info> tag were used.
  if (!$success || !$vals || $vals[0]['tag'] != 'PATTERN' || $vals[1]['tag'] != 'INFO') {
    form_set_error('xmlsource', t('Error parsing the XML, please check your syntax and try again.'));
  }
}

function patterns_import_submit($form, &$form_state) {
  if (isset($form_state['pattern_file'])) {
    $saved = file_copy($form_state['pattern_file']->filepath, $form_state['pattern_file']->destination);
  }
  elseif ($form_state['values']['xmlsource']) {
    $saved = file_save_data($form_state['values']['xmlsource'], variable_get('patterns_save_xml', 'patterns') . '/' . $form_state['values']['xmlname'] . '.xml', FILE_EXISTS_REPLACE);
  }

  if ($saved) {
    // Reload patterns
    patterns_get_patterns(true);
    drupal_set_message(t('Pattern successfully imported.'));
  }
  else {
    drupal_set_message(t('File couldn\'t be saved on the server. Import failed.'), 'error');
  }
  $form_state['redirect'] = 'admin/patterns';
}
