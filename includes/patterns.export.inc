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
  
  if(isset($form_state['values']['zip']) && !empty($form_state['values']['zip'] ))    
    $zip_flag = $form_state['values']['zip'];
  else
    $zip_flag = false;
  
  // check if we want to zip he files
  if ($zip_flag) {
    $zip = new ZipArchive();
    // TODO: check if path ok; get better path if possible
    $zip_filename = "./patterns" . strval(time()) . ".zip"; 
    if ($zip->open($zip_filename, ZIPARCHIVE::CREATE)!=TRUE) {
      exit("Cannot locally create zip-archive. Ask your administrator for help.");
    }
    $filename = "patterns" . strval(time()) . ".zip";
  } 
  
  // send header depending on if we zip or not
  if ($zip_flag) {
    drupal_add_http_header("Content-type", "application/octet-stream");
  } else {
    drupal_add_http_header("Content-type", " text/plain; charset=utf-8");
  }
  drupal_add_http_header("Content-Disposition", "attachment;filename=". $filename);
  
  // concatenate yaml files to a huge file or zip all files
  foreach ($form_state['values']['patterns_table'] as $pid) {
    if(!is_int($pid)) {
      $pattern = patterns_get_pattern($pid);
      
      if(substr($pattern->file, -4) != "yaml")
        continue;
      //if(!$pattern->public)
      //  continue;
      if ($zip_flag) {
        $zip->addFromString($pattern->name . '.yaml', file_get_contents($pattern->file));
      }
      else {
        $file .= "---\n";
        $file .=  file_get_contents($pattern->file);
        $file .= "\n";
      }
    }
  }

  if ($zip_flag) {
    $zip->close();
    $h = readfile($zip_filename);
    unlink($zip_filename);   
  }
  else {
    print $file;
  }
  exit;
}