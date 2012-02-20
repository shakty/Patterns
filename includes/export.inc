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
 patterns_load_components();

 //$removed = $args[PATTERNS_STATUS_REMOVED];

 // Load the patterns from database.
 $patterns = patterns_get_patterns();
 $patterns = $patterns[PATTERNS_STATUS_OK];

 foreach ($patterns as $pid => $pattern) {
  $category = @$pattern->info['category'] ? @$pattern->info['category'] : t('Other');
  $name = $pattern->title;
  $status = (@$pattern->status) ? t('Enabled') : t('Disabled');
  $author = @$pattern->info['author'];
  $version = @$pattern->info['version'];
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


 $form['patterns']['patterns_table'] = array
 (
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $rows,
    '#empty' => t('No Patterns'),
 );

 if (class_exists('ZipArchive')) {
  $form['zip'] = array(
    '#type' => 'checkbox',
    '#title' => 'download files as zip-archive',
  );
 }
 $form['submit'] = array(
   '#type' => 'submit',
   '#value' => t('Download')
 );
 $form['#submit'][] = 'patterns_export_submit';

 return $form;
}


/**
 * Exports selected patterns either in a file or as a zip-archive
 * @param $form
 * @param $form_state
 */
function patterns_export_submit($form, &$form_state) {
  @patterns_load_components();

  // Load the patterns from database.
  @$patterns = patterns_get_patterns();
  @$patterns = $patterns[PATTERNS_STATUS_OK];
  //$removed = $args[PATTERNS_STATUS_REMOVED];

  $file = "";
  $filename = "patterns_export.yaml";
  $path = patterns_config_get_files_dir();

  if (isset($form_state['values']['zip']) && !empty($form_state['values']['zip'] ))
    $zip_flag = $form_state['values']['zip'];
  else
    $zip_flag = FALSE;

  // check if we want to zip he files
  if ($zip_flag) {
    $zip = new ZipArchive();
    // TODO: check if path ok; get better path if possible
    $zip_filename = "patterns" . strval(time()) . ".zip";
    if ($zip->open($path . $zip_filename, ZIPARCHIVE::CREATE)!=TRUE) {
      exit("Cannot locally create zip-archive. Ask your administrator for help.");
    }
  }

  // send header depending on if we zip or not
  if ($zip_flag) {

  }
  else {

  }


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
      if (!is_readable($pattern->file)) {
        drupal_set_message("An error occurred: couldn't read " . $pattern->file . ".", 'error'); // TODO: t()
        continue;
      }
      if ($zip_flag) {
        $zip->addFromString($pattern->name . '.yaml', @file_get_contents($pattern->file));
      }
      else {
        $file .= "---\n";
        $file .=  @file_get_contents($pattern->file);
        $file .= "\n";
      }
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
  }
  else { // .yaml file
    drupal_add_http_header("Content-type", " text/plain; charset=utf-8");
    drupal_add_http_header("Content-Disposition", "attachment;filename=" . $filename);
    print $file;
    exit;
  }

}
