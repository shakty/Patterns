<?php

function patterns_load_yaml($path, $local = TRUE) {
  if ($local && !file_exists($path)) {
    return FALSE;
  }

  include_once drupal_get_path('module', 'patterns') . '/spyc/spyc.php';
  $pattern = Spyc::YAMLLoad($path);

  if (!patterns_validate_pattern($pattern)) {
    return FALSE;
  }

  return $pattern;
}

function patterns_load_string_yaml($source) {
  // loading yaml from source doesn't preserve line breaks
  // so we need to save it as a file first
  $path = file_directory_temp() . '/import.yaml';
  file_save_data($source, $path, FILE_EXISTS_REPLACE);
  $pattern = patterns_load_yaml($path);
  unlink($path);

  return $pattern;
}

function patterns_load_xml($path, $local = TRUE) {
  if ($local && !file_exists($path)) {
    return FALSE;
  }

  if (!$xml = file_get_contents($path)) {
    return FALSE;
  }

  return patterns_load_string_xml($xml);
}

function patterns_load_string_xml($source) {
  $pattern = patterns_from_source($source);
  if (empty($pattern) || $pattern['tag'] != 'pattern') {
    return FALSE;
  }

  // Rearrange the data in a nice way for each component.
  // Make sure actions are processed differently so order is preserved.
  $pattern = patterns_rearrange_data($pattern);

  foreach ($pattern as $key => $values) {
    $pattern[$values['tag']] = $values;
    unset($pattern[$values['tag']]['tag']);
    unset($pattern[$key]);
  }

  if (!patterns_validate_pattern($pattern)) {
    return FALSE;
  }

  return $pattern;
}

/**
 * Read and evaluate a php file to return a 'pattern'
 */
function patterns_load_php($path, $local = TRUE) {
  if ($local && !file_exists($path)) {
    return FALSE;
  }
  $pattern = array();

  @include($path);
  // That should have declared a 'pattern' into current scope.

  if (!patterns_validate_pattern($pattern)) {
    // TODO: t()
    trigger_error("Failed to evaluate a useful pattern from the input file $path. Pattern did not validate. May have been invalid syntax. ", E_USER_WARNING);
    return FALSE;
  }
  return $pattern;
}

/**
 * Create a pattern from an XML data source
 */
function patterns_from_source($xml) {
  $parse = drupal_xml_parser_create($xml);
  xml_parser_set_option($parse, XML_OPTION_CASE_FOLDING, 0);

  if (!xml_parse_into_struct($parse, $xml, $vals, $index)) {
    return FALSE;
  }

  // Create a multi-dimensional array representing the XML structure
  $pattern = current(_patterns_parse_tag($vals));

  return $pattern;
}


function patterns_save_pattern($pattern, $path = '', $name = '') {

  $title = $pattern['info']['title'];
  $description = $pattern['info']['description'];
  $author = $pattern['info']['author'];

  // Ste: was
  //if ($pid = db_result(db_query("SELECT pid FROM {patterns} WHERE name = '%s'", $name))) {s
  if ($pid = db_query("SELECT pid FROM {patterns} WHERE name = :s", array('s' => $name))->fetchField()) {
    // Ste: was
    //$updated = db_result(db_query("SELECT updated FROM {patterns} WHERE pid = '%d'", $pid));
    $updated = (db_query("SELECT updated FROM {patterns} WHERE pid = :d", array('d' => $pid))->fetchField());
    if (($new_updated = filemtime($path)) > $updated) {
      // Ste: was
      //db_query("UPDATE {patterns} SET pattern = '%s', title = '%s', file = '%s', updated = '%s', description = '%s' WHERE pid = %d", serialize($pattern), $title, $path, $new_updated, $description, $pid);
      $query_params = array('pattern' => serialize($pattern), 'title' => $title, 'file' => $path, 'updated' => $new_updated, 'descr' => $description, 'pid' => $pid);
      db_query("UPDATE {patterns} SET pattern = :pattern, title = :title, file = :file, updated = :updated, description = :descr WHERE pid = :pid", $query_params); 
    }
    else {
      // Ste: was
      //db_query("UPDATE {patterns} SET pattern = '%s', title = '%s', file = '%s', description = '%s' WHERE pid = %d", serialize($pattern), $title, $path, $description, $pid);
      $query_params = array('pattern' => serialize($pattern), 'title' => $title, 'file' => $path, 'descr' => $description, 'pid' => $pid);
      db_query("UPDATE {patterns} SET pattern = :pattern, title = :title, file = :file, description = :descr WHERE pid = :pid", $query_params); 
    }
  }
  else {
    // Ste: was
    //db_query("INSERT INTO {patterns} (name, status, file, updated, enabled, title, description, pattern) VALUES ( '%s', 0, '%s', '%s', 0, '%s', '%s', '%s')", $name, $path, time(), $title, $description, serialize($pattern));
    // TODO: time() ?= REQUEST_TIME
    $query_params = array('name' => $name, 'pattern' => serialize($pattern), 'title' => $title, 'file' => $path, 'descr' => $description, 'time' => time());
    db_query("INSERT INTO {patterns} (name, status, file, updated, enabled, title, description, pattern) VALUES ( :name, 0, :file, :time, 0, :title, :descr, :pattern)", $query_params);
  }
}

/**
 * Menu callback - returns source code of the requested pattern
 * if the pattern is public
 *
 * @param $pid
 *   pattern id
 * @return
 *   source code of the requested pattern
 */
function patterns_get_source($pid) {
  if (!is_numeric($pid)) {
    exit;
  }

  $pattern = patterns_get_pattern($pid);

  // make sure pattern is public (published)
  if (!$pattern->public) {
    exit;
  }

  $content_type = 'text/plain';
  if (substr($pattern->file, -4) == '.xml') {
    $content_type = 'text/xml';
  }

  drupal_add_http_header('Content-Type', $content_type . '; charset=utf-8');
  print file_get_contents($pattern->file);
  exit;
}

/**
 * Loads the Patterns handlers (component) from file system,
 * if they are not already loaded.
 * 
 */
function patterns_load_components() {
  static $loaded = FALSE;

  if ($loaded) {
    return;
  }

  //Not nedeed now.
  //require_once drupal_get_path('module', 'patterns') .'/patterns.inc';

  // Get a list of directories to search
  $paths = module_invoke_all('patterns_directory');

  foreach ($paths as $path) {
    // Ste: was
    //foreach (file_scan_directory($path . '/\/components/', '.\.inc$') as $file) {
    foreach (file_scan_directory($path . '/components/', '/\.inc$/') as $file) {
      // Ste: was
      //include_once $file->filename;
      include_once $file->uri;
    }
  }

  $loaded = TRUE;
}


function patterns_get_patterns($reset = TRUE) {
  patterns_load_components();

  if ($reset || !variable_get('patterns_loaded', FALSE)) {
    // Get a listing of enabled patterns
    $enabled = array();
    // Below SQL query commented out for the time being because it's preventing 
    // pattern code stored in the patterns table to be updated/refreshed after 
    // pattern file has been modified.
    // @todo: After it's confirmed that this is not causing any side effects this 
    // function has to be cleaned from all the references to $enabled array. 
//    $result = db_query('SELECT file FROM {patterns} WHERE status = 1');
//
//    while ($pattern = db_fetch_object($result)) {
//      $enabled[] = $pattern->file;
//    }

    $priority = array();
    $errors = array();
    
    // Get list of directories to scan for patterns
    $patterns_paths = patterns_paths();

    // get valid file extensions
    $mask = '/.\.(' . implode('|', patterns_file_types()) . ')$/';

    // prepare list of files/folders to be excluded
    // 'enabled' - Don't save enabled pattern backups
    $no_mask = array('.', '..', 'CVS', '.svn', 'enabled');

    foreach ($patterns_paths as $path) {
      foreach (file_scan_directory($path, $mask, $no_mask) as $file) {
        // Can't update existing patterns that are enabled
        if (in_array($file->filename, $enabled) || in_array($file->name, $priority)) {
          continue;
        }

        $priority[] = $file->name;

        // choose appropriate function based on the file extension
        // Ste was
        // $func = 'patterns_load_'. substr($file->basename, strlen($file->name) + 1);
        $func = 'patterns_load_' . substr($file->filename, strlen($file->name) + 1);
        
        
        // Load and save pattern
        // Ste: was
        //if ($pattern = $func($file->filename)) {
        if ($pattern = $func($file->uri)) {
          // Ste: was
          //patterns_save_pattern($pattern, $file->filename, $file->name);
          patterns_save_pattern($pattern, $file->uri, $file->name);
        }
        else {
          $errors[] = $file->filename;
        }
      }
    }

    variable_set('patterns_loaded', time());
  }

  
  $result = db_query('SELECT * FROM {patterns}');
  

  $messages = array();
  
  // Ste: was
  //while ($pattern = db_fetch_object($result)) {
  foreach ($result as $pattern) {
  
    // skip pattern if its file is missing
    if (!is_file($pattern->file)) continue;

    // skip pattern if loading failed and report that to the user
    if (in_array($pattern->file, $errors)) {
      $messages[] = t("Pattern couldn't be loaded from the file '%file'", array('%file' => $pattern->file));
      continue;
    }

    $patterns[$pattern->pid] = $pattern;
    $data = unserialize($pattern->pattern);
    $patterns[$pattern->pid]->pattern = $data;
    $patterns[$pattern->pid]->info = $data['info'];
  }

  if (!empty($messages)) {
    drupal_set_message(implode('<br/>', $messages) . '<br/>' . t('Make sure that above file(s) are readable and contain valid data.'), 'error');
  }

  // Ste: $patterns may have not been initialized
  return $patterns;
}

/**
 * Load the pattern from the db and return a stdClass obj, containing 
 * the pattern code as array (unsereliazed)
 * 
 * @param mixed $id Numeric id or alphanumeric name of the pattern
 * @return stdClass $pattern the Pattern obj containing the unsereliazed pattern 
 */
function patterns_get_pattern($id) {
  
  if (is_numeric($id)) {
    $pattern = db_query("SELECT * FROM {patterns} WHERE pid = :pid", array('pid' => $id))->FetchAll();
  }
  elseif (is_string($id)) {
    $pattern = db_query("SELECT * FROM {patterns} WHERE name = :name", array('name' => $id))->FetchAll();
  }

  if (!$pattern) return FALSE;

  //Pattern data is stored in serialized form in the db.
  $pattern[0]->pattern = unserialize($pattern[0]->pattern); 
  return $pattern[0];
}




/**
 * Check if a .htaccess file exists to prevent downloads of pattern files
 */
function _patterns_check_file_dir() {
  return FALSE;
  // Ste: was
  //$path = file_create_path(variable_get('patterns_save_xml', 'patterns'));
  
  // real_path(public) returns the path to the files dir
  $path = drupal_realpath('public://') . "/" . variable_get('patterns_save_xml', 'patterns');

  if (!is_file($path . '/.htaccess')) {
    $content = '# Prevent downloading site patterns
<FilesMatch "\.xml$">
  Order allow,deny
</FilesMatch>
';
    file_save_data($content, $path . '/.htaccess');
  }
}
