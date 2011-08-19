<?php

/**
 * Display the import pattern from server form
 */
function patterns_import_server($form, &$form_state) {
  if (empty($form_state['storage'])) {

    // Ste: was
    //$path = file_create_path(variable_get('patterns_save_file', 'patterns'));
    
    // real_path(public) returns the path to the files dir
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
    
    // real_path(public) returns the path to the files dir
    $path = drupal_realpath('public://') . "/" . variable_get('patterns_save_file', 'patterns');
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
 
  $form['#attributes']['enctype'] = 'multipart/form-data';
  $form['pattern_file'] = array(
    '#type' => 'file',
    '#title' => t('Upload Pattern File'),
    '#description' => t('Imported patterns are not executed until you run them manually.'),
    '#size' => 48
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  // Not for now
  //$form['#validate'][] = 'patterns_import_validate';
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


function patterns_import ($form, $form_state) {
  
  return patterns_editor('patterns_import_source' );
}

/**
 * Display the import pattern form
 */
function patterns_import_source($form, &$form_state) {
  
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Pattern Identifier'),
    '#description' => t('Machine readable name for the pattern. The actual title should be included in the pattern itself.'),
    '#required' => TRUE
  );
  $form['content'] = array(
    '#type' => 'textarea',
    '#rows' => 25,
    '#title' => t('Enter Pattern Source Code'),
    '#description' => t('Imported patterns are not executed until you run them manually.')
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );
  
  
  // No Validation for now
  $form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}

function patterns_import_validate($form, &$form_state) {
  
  $format = 'yaml';
  $path = drupal_realpath('public://') . "/" . variable_get('patterns_save_file', 'patterns');
  // Check Patterns Directory
  if (!file_prepare_directory($path)) {
    $message = t("Destination folder doesn't exist: !path<br />", array('!path' => $path));
    $link = l(t('status report'), 'admin/reports/status');
    $message .= t('You might want to check the !link to ensure your files directory exists and is writable.', array('!link' => $link));
    form_set_error('pattern_file', $message);
    return FALSE;
  }
  
  if (!$form_state['values']['content']) { // It is not an import_source
  
    $uri = $form_state['values']['pattern_uri'];
    
    if(empty($uri)){
      form_set_error('pattern_uri', t('Invalid file or url provided.'));
      return FALSE;
    }
    
    $format = pathinfo($uri, PATHINFO_EXTENSION);
    $validators = array(); // filter out the extension
    
    // Remote File
    if (stripos($url, 'http://') == 1) {
      if (!ini_get('allow_url_fopen')) {
        form_set_error('url', t('allow_url_fopen must be enabled in your php configuration in order to use this feature.'));
        return FALSE;
      }
      // Only YAML and XML extension accepted for remote files
      $validators = array('file_validate_extensions' => array('xml','yaml'));
    }
  
    // TODO: move it into the hook_submit ?
    if ($file = file_save_upload($uri, $validators)) {
      $form_state['values']['content'] = file_get_contents($file->uri); // was $file->filepath
      $form_state['pattern_uri'] = $file;
      $form_state['pattern_uri']->destination = $path;
    }
    else {
      form_set_error('pattern_uri', t('Failed to retreive the pattern specified: %uri. Check your file and try again. Notice: only yaml and xml files are accepted', array('%uri' => $uri)));
      return FALSE;
    }
  }
  
  // Name Validation
  $name = $form_state['values']['name'];
  $errors_on_name = _patterns_parser_analyze_pattern_name($name);
  if (!empty($errors_on_name)){
    form_set_error('name', t('Errors on the name of the pattern: ' . implode(', ', $errors_on_name)));
    return FALSE;  
  }
  
  if ($obj = patterns_db_load_pattern_from_name($name)) {
    form_set_error('name', t('A pattern file with the same name alrady exists: !name', array('!name' => $name)));
    return FALSE;  
  }
  
  // FORMAT Validation
  if ($format == 'yaml') {
    // YAML
    $patternscan = _patterns_scan_pattern($form_state['values']['content']); 
    if (!_patterns_validate_patternscan($patternscan)) {
      $errors_on_scan = _patterns_analyze_patternscan($patternscan);
      form_set_error('name', t('Error parsing the YAML file: ' . implode(', ', $errors_on_scan)));
    }
  }
  else { // XML or other
    if (!patterns_parser_validate_xml_pattern ($form_state['values']['content'])){
      form_set_error('xmlsource', t('Error parsing the XML, please check your syntax and try again.'));
    }
  }
  
  // TODO: Check what happens with different formats
  $form_state['values']['format'] = $format;
  
  return TRUE;
  
}

function patterns_import_submit($form, &$form_state) {
  
  $success = FALSE;
  $name = $form_state['values']['name'];
  
  if (!empty($name)) {
    $content = $form_state['values']['content'];
    $format = (isset($form_state['values']['format'])) ? $form_state['values']['format'] : 'yaml';
    
    
    // What is this?
    if (isset($form_state['pattern_file'])) {
      $success = file_copy($form_state['pattern_file']->filepath, $form_state['pattern_file']->destination);
    }
    
    elseif (!empty($content)) {
      $success = patterns_io_save_pattern ($content, $name, $format );
    }
  }
  
  if ($success) {
      drupal_set_message(t('Pattern successfully imported.'));
  }
  else {
    drupal_set_message(t('File couldn\'t be saved on the server. Import failed.'), 'error');
  }
  $form_state['redirect'] = 'admin/patterns';
}

//function patterns_import_submit_old($form, &$form_state) {
//  if (isset($form_state['pattern_file'])) {
//    $saved = file_copy($form_state['pattern_file']->filepath, $form_state['pattern_file']->destination);
//  }
//  elseif ($form_state['values']['xmlsource']) {
//    $saved = file_save_data($form_state['values']['xmlsource'], variable_get('patterns_save_file', 'patterns') . '/' . $form_state['values']['xmlname'] . '.xml', FILE_EXISTS_REPLACE);
//  }
//
//  if ($saved) {
//    // Reload patterns
//    patterns_get_patterns(true);
//    drupal_set_message(t('Pattern successfully imported.'));
//  }
//  else {
//    drupal_set_message(t('File couldn\'t be saved on the server. Import failed.'), 'error');
//  }
//  $form_state['redirect'] = 'admin/patterns';
//}