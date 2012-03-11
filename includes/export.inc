<?php
/**
 * @file
 * Functions related to exporting patterns.
 */

function patterns_export($form, &$form_state) {

  $header = array(
    'category' => array('data' => t('category'), 'field' => 'n.category', 'sort' => 'asc'),
    'name' => array('data' => 'name', 'field' => 'n.name'),
    'status' => array('data' => 'status', 'field' => 'n.status'),
    'author' => array('data' => 'author', 'field' => 'n.author'),
    'version' => array('data' => 'version', 'field' => 'n.version'),
    'description' => array('data' => 'description', 'field' => 'n.description')
  );
  $rows = array();
  
  // Load the patterns from database.
  // no check with file system
  $patterns = patterns_db_get_patterns();
  
  foreach ($patterns as $pid => $pattern) {
    $pattern_code = unserialize($pattern->pattern);
    $category = @$pattern_code['info']['category'] ? @$pattern_code['info']['category'] : t('Other');
    $name = $pattern->title;
    $status = (@$pattern->status) ? t('Enabled') : t('Disabled');
    $author = @$pattern_code['info']['author'];
    $version = @$pattern_code['info']['version'];
    $description = @$pattern->description;
    $row = array(
    'category' => $category,
    'name' => $name,
    'status' => $status,
    'author' => $author,
    'version' => $version,
    'description' => $description);
    $rows[$pid-1] = $row; // want to avoid one
  }
  
  $form['patterns'] = array(
    '#type' => 'fieldset',
    '#title' => t('Export / Merge Patterns'),
    '#description' => t('From this area you can select multiple patterns and join them together into a new one. The resulting pattern file can be imported into the database or be downloaded as a file.')
  );
  
  $form['patterns']['patterns_table'] = array (
    '#type' => 'tableselect',
    '#prefix' => '<div>' . t('For now, only YAML patterns can be selected and exported') . '</div>',
    '#header' => $header,
    '#options' => $rows,
    '#empty' => t('No Patterns'),
  );
  
  $form['export'] = array(
    '#type' => 'fieldset',
    '#title' => t('Export options'),
    '#collapsible' => TRUE, 
    '#collapsed' => TRUE,  
  );
  
  $form['export']['mode'] = array(
    '#type' => 'radios',
    '#options' => _patterns_export_get_exp_mode_options(),
    '#title' => t('Export mode'),
    '#default_value' => PATTERNS_EXPORT_MODE_FULL,
  );
  
  $form['export']['to'] = array(
    '#type' => 'radios',
    '#options' => _patterns_export_get_exp_to_options(),
    '#title' => t('Action'),
    '#default_value' => PATTERNS_EXPORT_TO_FILE,
  );
 
 
  $form['export_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name of the exported pattern file'),
    '#description' => t('Extension will be automatically added'),
    '#default_value' => t('exported_pattern') . '_' . strval(time()),
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Download')
  );
  
  $form['#submit'][] = 'patterns_export_submit';
  
  return $form;
}

/**
 * Checks if the patterns directory is ready and if the file name
 * for exported patterns is valid.
 * 
 * @param $form
 * @param $form_state
 */
function patterns_export_validate($form, &$form_state) {
  
  if (!isset($form_state['values']['patterns_table'])) {
    drupal_set_message(t('An error has occurred'), 'error');
    return FALSE;
  }
  
  if (count($form_state['values']['patterns_table']) === 0) {
    drupal_set_message(t('No pattern selected to export.'), 'error');
    return FALSE;
  }

  if (!isset($form_state['values']['mode']) || !patterns_export_is_valid_export_mode($form_state['values']['mode'])) {
    drupal_set_message(t('Please select a valid export mode option'), 'error');
    return FALSE;
  }
  
  if (!isset($form_state['values']['to'])) {
    drupal_set_message(t('Please select a valid action for the export'), 'error');
    return FALSE;
  }
  
  if ($form_state['values']['to'] === PATTERNS_EXPORT_TO_DB) {
    _patterns_imports_check_name($form_state['values']['export_name']);
  }
  else if (empty($form_state['values']['export_name'])) {
    $form_state['values']['export_name'] = 'exported_pattern';
  }

  return TRUE;
}

/**
 * Exports selected patterns either in a file or as a zip-archive
 * @param $form
 * @param $form_state
 */
function patterns_export_submit($form, &$form_state) {
  // Load the patterns from database.
  @$patterns = patterns_db_get_patterns();
  
  $filename = $form_state['values']['export_name'];
  $path = patterns_config_get_files_dir();

  $zip_flag = ($form_state['values']['to'] === PATTERNS_EXPORT_TO_ZIP) ? TRUE : FALSE;
  
  if ($zip_flag) {
    $zip = new ZipArchive();
    // TODO: check if path ok; get better path if possible
    $zip_filename = "patterns" . strval(time()) . ".zip";
    if ($zip->open($path . $zip_filename, ZIPARCHIVE::CREATE)!=TRUE) {
      exit("Cannot locally create zip-archive. Ask your administrator for help.");
    }
  }
  else {
    // the export array with a general info section
    $export = patterns_utils_generate_info_section($filename, 'Exported');
  }

  // Load SPYC
  require_once libraries_get_path('spyc') . '/spyc.php';
  
  $count = 0; // count how many patterns will be exported
  $errors = array();
  // concatenate yaml files to a single file or zip all files
  foreach ($form_state['values']['patterns_table'] as $pid) {
    if (!is_int($pid)) {
      $pattern = patterns_get_pattern($pid);
      
      if (!$pattern) {
        drupal_set_message(t('Error reading pattern with id :pid', array(':pid' => $pid)), 'error');
        continue;
      }
      
      if (substr($pattern->file, -4) != "yaml") {
        continue;
      }
      
      if (empty($pattern->pattern)) {
        $errors[] = patterns_utils_toString($pattern);
        continue;
      }

      if ($zip_flag) {
        $zip->addFromString($pattern->name . '.yaml', Spyc::YAMLDump($pattern->pattern));
      }
      else {
        $included = _patterns_export_include_pattern($pattern, $form_state['values']['mode']);
        $included = array('pattern' => $included);      
        $export['exported'][] = array('include' => $included);
      }
      
      $count++;
    }
  }

  if ($zip_flag) {
    $zip->close();
    $zip_path = $path . $zip_filename;
    if (!is_readable($zip_path)) {
      drupal_set_message("An error occured: " . $zip_path . " isn't readable.", 'error'); // TODO: t()
    }
    else {
      drupal_add_http_header("Content-type", "application/octet-stream");
      drupal_add_http_header("Content-Disposition", "attachment;filename=" . $zip_path);
      $err = @readfile($zip_path);
      if ($err == FALSE) {
        drupal_set_message("An error occured: couldn't read " . $zip_path . ".", 'error'); // TODO: t()
      }
      exit;
    }
    if (@drupal_unlink($zip_path) == FALSE) {
      drupal_set_message("An error occured: couldn't delete file " . $zip_path . ".", 'error'); // TODO: t()
    }
    return;
  }
  
  // Add an header to the exported pattern
  $file = "# Patterns exported: " . $count . " \n";
  $file.= "# Date and time: " . date('r') . "\n\n";
  $file.= Spyc::YAMLDump($export);

  if ($form_state['values']['to'] === PATTERNS_EXPORT_TO_DB) {
    patterns_io_save_pattern($file, $filename, 'yaml', $path);
    drupal_goto('admin/patterns/export/');
  }
  else {
    drupal_add_http_header("Content-type", " text/plain; charset=utf-8");
    drupal_add_http_header("Content-Disposition", "attachment;filename=" . $filename);
    print $file;
    exit;
  }
}

/**
 * Builds an associative array of export options. 
 * 
 * Checks if zip compression is available.
 * 
 */
function _patterns_export_get_exp_to_options() {
  $options = array(
    PATTERNS_EXPORT_TO_DB   => t('Import into the database'),
    PATTERNS_EXPORT_TO_FILE => t('Download as a pattern file'),
  );
  
  if (class_exists('ZipArchive')) {
    $options[PATTERNS_EXPORT_TO_ZIP] = t('Download as a zip-archive');
  }
  
  return $options;
}

/**
 * Builds an associative array of export options. 
 * 
 * Checks if zip compression is available.
 * 
 */
function _patterns_export_get_exp_mode_options() {
  $options = array(
    PATTERNS_EXPORT_MODE_FULL     => t('All the sections (info and modules included) of the pattern'),
    PATTERNS_EXPORT_MODE_ACTIONS  => t('Actions only (info and modules sections excluded) of the pattern'),
    PATTERNS_EXPORT_MODE_NAME     => t('The unique name of the pattern'),
    PATTERNS_EXPORT_MODE_ID       => t('The numerical id of the pattern'),
  );
  
  return $options;
}

/**
 * Returns TRUE if the passed parameter is a valid flag
 * to configure Patterns behaviors with the 'include' tag.
 *  
 */
function patterns_export_is_valid_export_mode($mode = NULL) {
  if (is_null($mode)) return FALSE;
  
  $modes = array(
    PATTERNS_EXPORT_MODE_FULL,
    PATTERNS_EXPORT_MODE_ACTIONS,
    PATTERNS_EXPORT_MODE_NAME,
    PATTERNS_EXPORT_MODE_ID,
  );
  
  return (in_array($mode, $modes)) ? TRUE : FALSE;
}

/**
 * Returns the portion of a pattern object to be included as a string,
 * depending on the passed mode of exporting. 
 * 
 * @param stdclass $pattern an object representing the pattern as loaded from
 *  the database
 * @param mixed $mode one of the four inclusion modes
 * 
 */
function _patterns_export_include_pattern($pattern, $mode = PATTERNS_EXPORT_MODE_FULL) {
  if (empty($pattern)) return FALSE;
  if ($mode === PATTERNS_EXPORT_MODE_FULL) {
    return $pattern->pattern;
  }
  if ($mode === PATTERNS_EXPORT_MODE_ACTIONS) {
    return patterns_parser_extract_all_actions($pattern->pattern);
  }
  if ($mode === PATTERNS_EXPORT_MODE_NAME) {
    return $pattern->name;
  }
  if ($mode === PATTERNS_EXPORT_MODE_ID) {
    return $pattern->pid;
  }
  return FALSE;
  
}

