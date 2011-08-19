<?php

function patterns_load_yaml($path, $local = TRUE) {
  if ($local && !file_exists($path)) {
    return FALSE;
  }

  include_once libraries_get_path('spyc') . '/spyc.php';
  $pattern = Spyc::YAMLLoad($path);

  if (!patterns_validate_pattern($pattern)) {
    return FALSE;
  }

  return $pattern;
}

//function patterns_load_string_yaml($source) {
//  // loading yaml from source doesn't preserve line breaks
//  // so we need to save it as a file first
//  $path = file_directory_temp() . '/import.yaml';
//  file_save_data($source, $path, FILE_EXISTS_REPLACE);
//  $pattern = patterns_load_yaml($path);
//  unlink($path);
//
//  return $pattern;
//}

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

  // Get a list of directories to search
  $paths = module_invoke_all('patterns_directory');

  foreach ($paths as $path) {
    foreach (file_scan_directory($path . '/components/', '/\.inc$/') as $file) {
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

        // choose appropriate function based on the file extension: YAML or XML
        $func = 'patterns_load_' . substr($file->filename, strlen($file->name) + 1);
             
        // Load and save pattern
        if ($pattern = $func($file->uri)) {
          patterns_db_save_pattern($pattern, $file->uri, $file->name);
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

function patterns_io_get_patterns_service() {
  return 'Ciao';
}

/**
 * Load the pattern from the db and return a stdClass obj, containing 
 * the pattern code as array (unserialized)
 * 
 * @param mixed $id Numeric id or alphanumeric name of the pattern
 * @return stdClass $pattern the Pattern obj containing the unsereliazed pattern 
 */
function patterns_get_pattern($id) {
  
  $pattern = FALSE;
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


/**
 * 
 * @param unknown_type $pattern
 * @param unknown_type $format
 * 
 * @TODO: move $format as an element of pattern, a column in the db
 * 
 */
function patterns_io_save_pattern ($content, $name, $format = 'yaml') {
  
  // System Patterns Directory
  $dir = drupal_realpath('public://') . "/" . variable_get('patterns_save_file', 'patterns');
  
  if (file_prepare_directory($dir, TRUE)) {
    $path =  $dir . '/' . $name . '.' . $format;

    if (file_unmanaged_save_data($content, $path, FILE_EXISTS_REPLACE)) {
  
      // Load and save pattern
      $load_func = 'patterns_load_' . $format; 
      
      if ($pattern = $load_func($path)) {      
        patterns_db_save_pattern($pattern, $path, $name);
        drupal_set_message(t('%name was saved.', array('%name' => $name)));
        // Not for now
        //_patterns_clear_failed_edit();
      }
      else {
        drupal_set_message(t("Pattern '%name' could not be saved into the database. Make sure edited code is well-formed.", array('%name' => $name)), 'error');
        return FALSE;
      }
    }
    else{
      drupal_set_message(t('Error: file could not be saved.'), 'error');
      return FALSE;
    }
  }
  else {
    drupal_set_message(t('Error: the pattern is not writable. Please check the file system permissions.'), 'error');
    return FALSE;
  }
  
  
  return TRUE;
}

function patterns_io_remove_pattern($path){
  
}


