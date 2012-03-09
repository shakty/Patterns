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
  $description = t(@$pattern->description);
  $row = array(
  'category' => $category,
  'name' => $name,
  'status' => $status,
  'author' => $author,
  'version' => $version,
  'description' => $description);
  $rows[$pid-1] = $row; // want to avoid one
 }

 $form['patterns']['patterns_table'] = array (
    '#type' => 'tableselect',
    '#prefix' => '<div>' . t('For now, only YAML patterns can be selected and exported') . '</div>',
    '#header' => $header,
    '#options' => $rows,
    '#empty' => t('No Patterns'),
 );

 if (class_exists('ZipArchive')) {
  $form['zip'] = array(
    '#type' => 'checkbox',
    '#title' => t('Download files as a zip-archive'),
  );
 }

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
  
  if (empty($form_state['values']['export_name'])) {
    $form_state['values']['export_name'] = 'exported_pattern';
  } 
  $form_state['values']['export_name'] = $form_state['values']['export_name'] . '.yaml';
  
  return TRUE;
}

/**
 * Exports selected patterns either in a file or as a zip-archive
 * @param $form
 * @param $form_state
 */
function patterns_export_submit($form, &$form_state) {
  //@patterns_load_components();

  // Load the patterns from database.
  @$patterns = patterns_db_get_patterns();
  
  $filename = $form_state['values']['export_name'];
  $path = patterns_config_get_files_dir();

  if (isset($form_state['values']['zip']) && !empty($form_state['values']['zip'] )) {
    $zip_flag = $form_state['values']['zip'];
  }
  else {
    $zip_flag = FALSE;
  }
  
  // check if we want to zip he files
  if ($zip_flag) {
    $zip = new ZipArchive();
    // TODO: check if path ok; get better path if possible
    $zip_filename = "patterns" . strval(time()) . ".zip";
    if ($zip->open($path . $zip_filename, ZIPARCHIVE::CREATE)!=TRUE) {
      exit("Cannot locally create zip-archive. Ask your administrator for help.");
    }
  }

  $count = 0; // count how many patterns will be exported
  $errors = array();
  
  // the export array with a general info section
  $export = patterns_utils_generate_info_section($filename, 'Exported');

  // concatenate yaml files to a huge file or zip all files
  foreach ($form_state['values']['patterns_table'] as $pid) {
    if (!is_int($pid)) {
      $pattern = patterns_get_pattern($pid);
      if (!$pattern) {
        drupal_set_message(t('Error reading pattern with id :pid', array(':pid' => $pid)), 'error');
        continue;
      }
      if (substr($pattern->file, -4) != "yaml")
        continue;
      //if(!$pattern->public)
      //  continue;

      if (!$pattern->pattern) {
        $errors[] = patterns_utils_toString($pattern);
        continue;
      }

      // unserialize pattern code
      //$pattern_code = unserialize($pattern->pattern); 
      //debug($pattern_code);
      // add it to the export array
      
      $included = array('pattern' => $pattern->pattern);
      
      $export['exported'][] = array('include' => $included);
      
//      if (!is_readable($pattern->file)) {
//        drupal_set_message("An error occurred: couldn't read " . $pattern->file . ".", 'error'); // TODO: t()
//        continue;
//      }

      if ($zip_flag) {
        $zip->addFromString($pattern->name . '.yaml', @file_get_contents($pattern->file));
      }
      else {
//        $file .= "\n################## \n";
//        $file .= "\ninclude:\n\n";
//        $file .= @file_get_contents($pattern->file);
//        $file .= "\n";
      }
      
      $count++;
    }
  }
 
  // Add an header to the exported pattern
//  $header = "# Patterns exported: " . $count . " \n";
//  $header.= "# Date and time: " . date('r') . "\n\n";
//  $file = $header . $file;
  
  // Load SPYC
  require_once libraries_get_path('spyc') . '/spyc.php';
  $file = Spyc::YAMLDump($export);
  

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
  }
  else { // .yaml file
    drupal_add_http_header("Content-type", " text/plain; charset=utf-8");
    drupal_add_http_header("Content-Disposition", "attachment;filename=" . $filename);
    print $file;
    exit;
  }

}
