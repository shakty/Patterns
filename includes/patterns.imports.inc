<?php
/**
 * @file
 * Importing Patterns from a file or using an URL.
 */

/**
 * Form constructor for the Pattern import form.
 *
 * @see patterns_import_validate()
 * @see patterns_import_submit()
 * @ingroup forms
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

  $form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}

/**
 * Display the import pattern file form
 */
function patterns_import_file(&$form_state) {

  $form['#attributes']['enctype'] = 'multipart/form-data';

  $form['pattern_uri'] = array(
    '#type' => 'file',
    '#title' => t('Upload Pattern File'),
    '#description' => t('Imported patterns are not executed until you run them manually.'),
    '#size' => 48
  );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Pattern Identifier'),
    '#description' => t('Machine readable name for the pattern. The actual title should be included in the pattern itself.'),
    '#required' => TRUE
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  //$form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}

/**
 * Display the import pattern url form
 */
function patterns_import_url(&$form_state) {

  $form['pattern_uri'] = array(
    '#type' => 'textfield',
    '#title' => t('Specify an URI'),
    '#description' => t('Import a pattern from a remote URL. Imported patterns are not executed until you run them manually.'),
    '#size' => 48
  );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Pattern Identifier'),
    '#description' => t('Machine readable name for the pattern. The actual title should be included in the pattern itself.'),
    '#required' => TRUE
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  // Now it has his own
  //$form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}


/**
 * Validate handler for file importing
 */
function patterns_import_file_validate($form, &$form_state) {

  // If not automatically returns
  _patterns_imports_check_dirs();
  _patterns_imports_check_name($form_state['values']['name']);


  // arg must be a string with a space separated list of allowed extensions.
  $validators = array('file_validate_extensions' => array('xml yaml'));

  if ($file = file_save_upload('pattern_uri', $validators)) {
    $form_state['values']['content'] = file_get_contents($file->uri); // was $file->filepath
    $form_state['pattern_uri'] = $file;
    $form_state['pattern_uri']->destination = drupal_realpath('public://') . "/" . variable_get('patterns_save_file', 'patterns'); // TODO: move this out for performance
  }
  else {
    form_set_error('pattern_uri', t('Failed to retreive the pattern specified: %uri. Check your file and try again. Notice: only yaml and xml files are accepted', array('%uri' => $uri)));
    return FALSE;
  }

  $format = pathinfo($form_state['pattern_uri']->uri, PATHINFO_EXTENSION);

  // TODO: PHP format
  if ($format == 'yaml') {
    // YAML
    $patternscan = _patterns_scan_pattern($form_state['values']['content']);
    if (!_patterns_validate_patternscan($patternscan)) {
      $errors_on_scan = _patterns_analyze_patternscan($patternscan);
      form_set_error('name', t('Error parsing the YAML file: ' . implode(', ', $errors_on_scan)));
    }
  }
  else { // XML or other
    if (!patterns_parser_validate_xml_pattern($form_state['values']['content'])) {
      form_set_error('xmlsource', t('Error parsing the XML, please check your syntax and try again.'));
    }
  }

  // TODO: Check what happens with different formats.
  $form_state['values']['format'] = $format;

  return TRUE;
}

function patterns_import_validate($form, &$form_state) {

  $format = 'yaml';

  // If not automatically returns
  _patterns_imports_check_dirs();
  _patterns_imports_check_name($form_state['values']['name']);

  if (!isset($form_state['values']['content'])) { // It is not an import_source
    $uri = $form_state['values']['pattern_uri'];

    if (empty($uri)) {
      form_set_error('pattern_uri', t('Invalid file or url provided.'));
      return FALSE;
    }

    $format = pathinfo($uri, PATHINFO_EXTENSION);
    $validators = array(); // filter out the extension

    // Remote File
    if (stripos($uri, 'http://') == 1) {
      if (!ini_get('allow_url_fopen')) {
        form_set_error('url', t('allow_url_fopen must be enabled in your php configuration in order to use this feature.'));
        return FALSE;
      }
      // Only YAML and XML extension accepted for remote files
      $validators = array('file_validate_extensions' => array('xml', 'yaml'));
    }

    // TODO: move it into the hook_submit ?
    if ($file = file_save_upload($uri, $validators)) {
      $form_state['values']['content'] = file_get_contents($file->uri); // was $file->filepath
      $form_state['pattern_uri'] = $file;
      $form_state['pattern_uri']->destination = drupal_realpath('public://') . "/" . variable_get('patterns_save_file', 'patterns'); // TODO: move this out for performance
    }
    else {
      form_set_error('pattern_uri', t('Failed to retreive the pattern specified: %uri. Check your file and try again. Notice: only yaml and xml files are accepted', array('%uri' => $uri)));
      return FALSE;
    }
  }

//  // Name Validation
//  $name = $form_state['values']['name'];
//  $errors_on_name = _patterns_parser_analyze_pattern_name($name);
//  if (!empty($errors_on_name)){
//    form_set_error('name', t('Errors on the name of the pattern: ' . implode(', ', $errors_on_name)));
//    return FALSE;
//  }
//
//  if ($obj = patterns_db_load_pattern_from_name($name)) {
//    form_set_error('name', t('A pattern file with the same name alrady exists: !name', array('!name' => $name)));
//    return FALSE;
//  }

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
    if (!patterns_parser_validate_xml_pattern($form_state['values']['content'])) {
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
      $success = patterns_io_save_pattern($content, $name, $format );
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

/**
 * Helper function to be called in one of the import validation handlers.
 */
function _patterns_imports_check_dirs() {
  // Check Patterns Directory
  if (!_patterns_io_is_patterns_dir_ready()) {
    $message = t("Destination folder doesn't exist: !path<br />", array('!path' => patterns_config_get_files_dir())); // TODO: move this out for performance
    $link = l(t('status report'), 'admin/reports/status');
    $message .= t('You might want to check the !link to ensure your files directory exists and is writable.', array('!link' => $link));
    form_set_error('pattern_file', $message);
    return FALSE;
  }
}

/**
 * Helper function to be called in one of the import validation handlers.
 */
function _patterns_imports_check_name($name = '') {
  // Name Validation
  $errors_on_name = _patterns_parser_analyze_pattern_name($name);
  if (!empty($errors_on_name)) {
    form_set_error('name', t('Errors on the name of the pattern: ' . implode(', ', $errors_on_name)));
    return FALSE;
  }

  if ($obj = patterns_db_load_pattern_from_name($name)) {
    form_set_error('name', t('A pattern file with the same name alrady exists: !name', array('!name' => $name)));
    return FALSE;
  }
}
