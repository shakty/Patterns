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

  $def_dir = patterns_path_get_files_dir();

  $text = t('Cut and paste the content of a pattern file in the text area below to import it in the database.');
  $title = t('Import from Source');
  patterns_forms_add_page_header($form, $title, $text);

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Pattern Identifier'),
    '#description' => t('The new name of the pattern file. If the extension is missing it will be automatically added.'),
    '#required' => TRUE,
  );

  $form = patterns_forms_get_formats_selector($form);

  $form['validation_result'] = array(
    '#markup' => 'This pattern has not been validated yet.',
    '#prefix' => '<div id="validation_result"><strong>',
    '#suffix' => '</strong></div>',
  );

  $form['content'] = array(
    '#type' => 'textarea',
    '#rows' => 25,
    '#title' => t('Enter Pattern Source Code'),
    '#description' => t('Imported patterns are not executed until you run them manually.'),
  );

  // TODO: transform in a FILE form
  $form['destination'] = array(
    '#type' => 'textfield',
    '#title' => t('Destination'),
    '#description' => t('Pattern will be saved in this directory. Must be writable. Default dir: @dir', array('@dir' => $def_dir)),
    '#default_value' => $def_dir,
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  $form['validate'] = array(
    '#prefix' => '<span style=\'margin-right:10px\'>',
    '#markup' => "<a href='#' id='validate_pattern'>" . t('Validate') . "</a>",
    '#suffix' => '</span> ',
  );

  // ? Double array ?
  $form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}

/**
 * Display the import pattern file form
 */
function patterns_import_file($form, &$form_state) {

  $def_dir = patterns_path_get_files_dir();

  $text = 'Choose a local file to import in the database.';
  $title = 'Import from File';
  patterns_forms_add_page_header($form, $title, $text);

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
    '#description' => t('The new name of the pattern file. If the extension is missing it will be automatically added.'),
    '#required' => TRUE
  );

  $form = patterns_forms_get_formats_selector($form,
                                              PATTERNS_FORMAT_UNKNOWN,
                                              'If unknown, it will be determined by the extension.'
  );

  // TODO: transform in a FILE form
  $form['destination'] = array(
    '#type' => 'textfield',
    '#title' => t('Destination'),
    '#description' => t('Pattern will be saved in this directory. Must be writable. Default dir: @dir', array('@dir' => $def_dir)),
    '#default_value' => $def_dir,
    '#required' => TRUE,
  );

  $form['local'] = array(
    '#type' => 'hidden',
    '#value' => TRUE,
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
function patterns_import_url($form, &$form_state) {

  $text = 'Insert the uri of a remote file to be downloaded and imported in the database.';
  $title = 'Import from URL';
  patterns_forms_add_page_header($form, $title, $text);

  if (!ini_get('allow_url_fopen')) {
    $form['import_url'] = array(
    // The string 'Feature disabled' is checked inside the test directory
    // do not change or change both
    '#prefix' => '<strong>' . t('Feature disabled:') . '</strong>',
    '#markup' => t('directive "allow_url_fopen" must be enabled in your PHP configuration in order to use this feature.'),
    );
    return $form;
  }

  $def_dir = patterns_path_get_files_dir();

  $form['pattern_uri'] = array(
    '#type' => 'textfield',
    '#title' => t('Specify an URI'),
    '#description' => t('Import a pattern from a remote URL. Imported patterns are not executed until you run them manually.'),
    '#size' => 48
  );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Pattern Identifier'),
    '#description' => t('The new name of the pattern file. If the extension is missing it will be automatically added.'),
    '#required' => TRUE
  );

  $form = patterns_forms_get_formats_selector($form,
                                              PATTERNS_FORMAT_UNKNOWN,
                                              'If unknown, it will be determined by the extension.'
  );

  $form['destination'] = array(
    '#type' => 'textfield',
    '#title' => t('Destination'),
    '#description' => t('Pattern will be saved in this directory. Must be writable. Default dir: @dir', array('@dir' => $def_dir)),
    '#default_value' => $def_dir,
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  // Now it has his own
  $form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}

function patterns_import_validate($form, &$form_state) {

  $destination = $form_state['values']['destination'];

  // If not automatically returns
  _patterns_import_check_dirs($destination);
  _patterns_import_check_name($form_state['values']['name']);

  // Retreiving the format
  if (isset($form_state['values']['format']) && $form_state['values']['format'] != PATTERNS_FORMAT_UNKNOWN) {
    $format = $form_state['values']['format'];
  }
  elseif (isset($_FILES['files']['name']['pattern_uri'])) {
    $format = pathinfo($_FILES['files']['name']['pattern_uri'], PATHINFO_EXTENSION);
  }
  elseif (isset($form_state['values']['pattern_uri'])) {
    $format = pathinfo($form_state['values']['pattern_uri'], PATHINFO_EXTENSION);
  }
  else {
    $format = PATTERNS_FORMAT_UNKNOWN;
  }

  if (!patterns_parser_exists($format)) {
    form_set_error('format', t('The pattern format seems to be invalid.'));
    return FALSE;
  }

  // Safety assign
  $form_state['values']['format'] = $format;

  // FILE or URI
  if (!isset($form_state['values']['content'])) {

    $err_file = FALSE;
    // Local
    if (isset($form_state['values']['local'])) {
      $validators = array('file_validate_extensions' => array(implode(' ', patterns_parser_get_formats(FALSE))));
      if ($file = file_save_upload('pattern_uri', $validators)) {
        $form_state['values']['content'] = file_get_contents($file->uri);
        $form_state['pattern_uri'] = $file;
        $form_state['pattern_uri']->destination = $destination;
      }
      else {
        $err_file = TRUE;
      }
    }
    // Remote File
    else {

      if (!ini_get('allow_url_fopen')) {
        form_set_error('url', t('allow_url_fopen must be enabled in your php configuration in order to use this feature.'));
        return FALSE;
      }

      if ($content = file_get_contents($form_state['values']['pattern_uri'])) {
        $form_state['values']['content'] = $content;
      }
      else {
        $err_file = TRUE;
      }
    }

    if ($err_file) {
      form_set_error('pattern_uri', t('Failed to retrieve the pattern specified: %uri. Check your file and try again.', array('%uri' => $form_state['values']['pattern_uri'])));
      return FALSE;
    }
  }
  // End FILE or URI



  // Parse
  $pattern = patterns_parser_parse($form_state['values']['content'], $format);

  if (!$pattern) {
    form_set_error('content', t('Pattern could not parsed. Please make sure it is well formed.'));
    return FALSE;
  }


  $patternscan = patterns_scan_pattern($pattern);
  if (!_patterns_scan_validate_patternscan($patternscan)) {
    $errors_on_scan = _patterns_scan_analyze_patternscan($patternscan);
    form_set_error('name', t('Error parsing the YAML file: ' . implode(', ', $errors_on_scan))); // TODO: proper t()
  }


  // We save here the parsed pattern
  $form_state['values']['pattern'] = $pattern;


  return TRUE;

}

function patterns_import_submit($form, &$form_state) {

  $success = FALSE;
  $name = $form_state['values']['name'];
  $format = (isset($form_state['values']['format'])) ? $form_state['values']['format'] : PATTERNS_FORMAT_UNKNOWN;

  // Add the extension if missing or invalid
  if (!_patterns_io_file_has_valid_extension($name)) {
    $name = $name . '.' . $format;
  }

  $content = (isset($form_state['values']['pattern'])) ? $form_state['values']['pattern']
                                                      : $form_state['values']['content'];

  if (!empty($content)) {
    $success = patterns_io_save_pattern($content,
                                        $name,
                                        $format,
                                        $form_state['values']['destination'],
                                        $form_state['values']['content']
    );
  }

  if ($success) {
    $link = l('here', 'admin/patterns/edit/' . $name); // TODO: t()
    $msg = t('Pattern %name successfully imported. ', array('%name' => $name)); // TODO: proper t()
    $msg.= t('Click ') . $link . t(' to view it.'); // TODO: t()
    drupal_set_message($msg);
  }
  else {
    drupal_set_message(t('File %name could not be saved on the server. Import failed.', array('%name' => $name)), 'error');
  }
  $form_state['redirect'] = 'admin/patterns';
}


/**
 * Helper function to be called in one of the import validation handlers.
 */
function _patterns_import_check_dirs($dir = NULL) {
  // Check Patterns Directory.
  if (!_patterns_io_is_patterns_dir_ready($dir, FILE_CREATE_DIRECTORY)) {
    $message = t("Destination folder does not exist or is not writable: %path<br />", array('%path' => $dir)); // TODO: move this out for performance
    $link = l(t('status report'), 'admin/reports/status');
    $message .= t('You might want to check the !link to ensure your files directory exists and is writable.', array('!link' => $link));
    form_set_error('pattern_file', $message);
    return FALSE;
  }
}

/**
 * Helper function to be called in one of the import validation handlers.
 */
function _patterns_import_check_name($name = '') {
  // Name Validation.
  $errors_on_name = _patterns_parser_analyze_pattern_name($name);
  if (!empty($errors_on_name)) {
    form_set_error('name', t('Errors in the name of the pattern: ' . implode(', ', $errors_on_name))); // TODO: use proper t()
    return FALSE;
  }

  if ($obj = patterns_db_load_pattern_from_name($name)) {
    form_set_error('name', t('A pattern file with the same name already exists: !name', array('!name' => $name)));
    return FALSE;
  }
}

/**
 * Creates a miniminal form_state object to import a pattern into the database.
 *
 * Returns FALSE if $name or $pattern are missing.
 *
 * @param mixed $name the name of the pattern
 * @param mixed $pattern the pattern code as string or array. If array gets
 *  converted to string
 * @param mixed $dest the folder in the file system where the pattern will be
 *  saved. If empty it uses the default patterns files directory.
 */
function patterns_import_build_import_form_state($name = NULL, $pattern = NULL, $dest = NULL) {
  if (is_null($name) || is_null($pattern)) return FALSE;
  if (is_null($dest)) {
    $dest = patterns_path_get_files_dir();
  }

  $form = array();
  $form['values'] = array();
  $form['values']['name']         = $name;
  $form['values']['content']      = $pattern;
  $form['values']['destination']  = $dest;
  return $form;
}

