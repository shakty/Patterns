<?php
/**
 * @file
 * Functions related to input/output operations.
 */

/**
 * Loads and validates a YAML pattern file.
 *
 * @param string $path The path of the pattern file.
 * @param bool $local
 *   (optional) TRUE if the pattern file is local. Defaults to TRUE.
 *
 * @return array|bool
 *   The pattern structure, or FALSE.
 */
function patterns_load_yaml($path, $local = TRUE) {
  if ($local && !file_exists($path)) {
    return FALSE;
  }

  require_once libraries_get_path('spyc') . '/spyc.php';
  return Spyc::YAMLLoad($path);
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

/**
 * Loads an XML pattern file without validating.
 *
 * @param string $path The path of the pattern file.
 * @param bool $local
 *   (optional) TRUE if the pattern file is local. Defaults to TRUE.
 *
 * @return array|bool
 *   The pattern structure, or FALSE.
 */
function patterns_load_xml($path, $local = TRUE) {
  if ($local && !file_exists($path)) {
    return FALSE;
  }

  if (!$xml = file_get_contents($path)) {
    return FALSE;
  }

  return patterns_load_string_xml($xml);
}

/**
 * @TODO
 */
function patterns_load_string_xml($source) {
  $pattern = patterns_from_source($source);
  if (empty($pattern) || $pattern['tag'] != 'pattern') {
    // TODO: Error reporting.
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
 * Loads and validates a PHP pattern file.
 *
 * @param string $path The path of the pattern file.
 * @param bool $local
 *   (optional) TRUE if the pattern file is local. Defaults to TRUE.
 *
 * @return array|bool
 *   The pattern structure, or FALSE.
 */
function patterns_load_php($path, $local = TRUE) {
  if ($local && !file_exists($path)) {
    return FALSE;
  }
  $pattern = array();

  include($path);
  // That should have declared a 'pattern' into current scope.

  if (!patterns_validate_pattern($pattern)) {
    // TODO: why not drupal_set_message?
    trigger_error(t("Failed to evaluate a useful pattern from the input file %path. Pattern did not validate. May have been invalid syntax.", array('%path' => $path)), E_USER_WARNING);
    return FALSE;
  }
  return $pattern;
}

/**
 * Creates a pattern from an XML data source.
 *
 * @param string $xml The XML string of the data source.
 *
 * @return array|bool
 *   The pattern structure, or FALSE.
 */

function patterns_from_source($xml) {
  $parse = drupal_xml_parser_create($xml);
  xml_parser_set_option($parse, XML_OPTION_CASE_FOLDING, 0);

  if (!xml_parse_into_struct($parse, $xml, $vals, $index)) {
    // TODO: Report parsing or other error.
    return FALSE;
  }

  // Create a multi-dimensional array representing the XML structure.
  $pattern = current(_patterns_parse_tag($vals));

  return $pattern;
}

function patterns_rearrange_data($pattern) {
  foreach ($pattern as $key => $value) {
    if (is_string($key)) {
      unset($pattern[$key]);
    }
    else {
      if ($value['tag'] == 'actions') {
        $pattern[$key] = patterns_rearrange_data($value);
        $pattern[$key]['tag'] = 'actions';

        // The refactored version, introduced in 7.x.
        // TODO: change _patterns_rearrange_data instead.
        // TODO: this does not actually work yet.
        $p = &$pattern[$key];
        foreach ($p as $k => $v) {
          if (is_string($k)) {
            continue;
          }
          $tag = $v['tag'];
          if (in_array($tag, array('create', 'modify', 'delete'))) {
            unset($p[$k]['tag']);
            $p[$k] = array($tag => $p);
          }
          else {
            // TODO: Error reporting: invalid tag, none of {create, modify, delete}.
          }
        }
      }
      else {
        $pattern[$key] = _patterns_rearrange_data($value);
      }
    }
  }

  return $pattern;
}

function _patterns_rearrange_data($data, $parent = '') {
  $numeric = array();
  $count = 0;

  foreach ($data as $key => $value) {
    if (isset($value['value'])) {
      if ($value['value'] == 'false') {
        $value['value'] = FALSE;
      }
      elseif ($value['value'] == 'true') {
        $value['value'] = TRUE;
      }
    }

    if (is_numeric($key) && is_array($value) && count($value) == 2 && isset($value['tag']) && isset($value['value'])) {
      unset($data[$key]);
      if (isset($data[$value['tag']])) {
        $numeric[] = $value['tag'];
        $data[$count++] = $data[$value['tag']];
        $data[$count++] = $value['value'];
        unset($data[$value['tag']]);
      }
      elseif (in_array($value['tag'], $numeric)) {
        $data[$count++] = $value['value'];
      }
      else {
        $data[$value['tag']] = $value['value'];
      }
    }
    elseif (is_numeric($key)) {
      $tag = $value['tag'];
      unset($value['tag']);
      $data[$tag][] = _patterns_rearrange_data($value, $tag);
      unset($data[$key]);
    }
  }

  foreach ($data as $key => $value) {
    if (is_array($value) && count($value) == 1 && $value[0]) {
      $data[$key] = $data[$key][0];
    }
  }

  // This workaround enables us to define numeric keys in XML by
  // prefixing the number with single character. E.g <n0>value</n0>
  // will result in 0 => 'value' (first character of the key will be removed).
  if (isset($data['_numeric_keys'])) {
    unset($data['_numeric_keys']);
    foreach ($data as $key => $value) {
      $data[substr($key, 1)] = $value;
      unset($data[$key]);
    }
  }

  return $data;
}

/**
 * Menu callback, returns source code of the requested pattern
 * if the pattern is public.
 *
 * @param integer $pid
 *   The ID of the Pattern to be displayed.
 */
function patterns_get_source($pid) {
  if (!is_numeric($pid)) {
    exit;
  }

  $pattern = patterns_get_pattern($pid);

  // Make sure pattern is public (published).
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
 * Loads the Patterns handlers (component) from the filesystem,
 * if they are not already loaded.
 */
function patterns_load_components() {
  static $loaded = FALSE;

  if ($loaded) {
    return;
  }

  // Get list of directories to scan for components.
  $paths = module_invoke_all('patterns_components');

  foreach ($paths as $path) {
    foreach (file_scan_directory($path, '/\.inc$/') as $file) {
      require_once $file->uri;
    }
  }

  $loaded = TRUE;
}

/**
 * Scan directories looking for patterns files.
 *
 * Checks inside the directories defined by patterns_config_get_paths().
 *
 * @param boolean $verbose if TRUE, displays on the screen information
 *   about the scan.
 *
 * @see patterns_config_get_paths()
 * @see patterns_io_analyze_scandir_messages()
 *
 * @return array An associative array of informative messages
 */
function patterns_io_scan_directories($verbose = FALSE) {
  
  $existing = patterns_db_get_patterns_array('name', TRUE);    
  // Patterns already enabled won't be updated in the database
  $enabled = patterns_db_get_enabled_patterns_array('name', TRUE);
  
  $messages = array();
  $messages['errors'] = array();
  $messages['skipped'] = array();
  $messages['permission'] = array();
  $messages['found'] = array();
  $messages['updated'] = array();
  $messages['error_found'] = array();
  $messages['error_updated'] = array();

  // Get list of directories to scan for patterns.
  $patterns_paths = patterns_config_get_paths();

  // Get valid file extensions.
  $mask = '/.\.(' . implode('|', patterns_config_get_file_types()) . ')$/';

  // Prepare list of files/folders to be excluded.
  // 'enabled' - Don't save enabled pattern backups.
  $no_mask = array('.', '..', 'CVS', '.svn', '.git', '.bzr', 'enabled');

  foreach ($patterns_paths as $path) {
    //debug($path);
    foreach (file_scan_directory($path, $mask, $no_mask) as $file) {
      
      if (in_array($file->name, $existing)) {
        if (!variable_get('patterns_update_db_from_fs', TRUE)) {
          continue;
        }
        
        if (!patterns_db_is_pattern_updated($file->name)) {
          // the database is already updated to the most recent version
          continue;
        }
        
        if (!empty($enabled)) {
          if (in_array($file->name, $enabled)) {
            $messages['skipped'][] = $file->name;
            continue; // Skip updating enabled patterns.
          }
        }
      }
  
      if (!is_readable($file->uri)) {
        $messages['permission'][] = $file->filename;
        continue;
      }
      
      // Choose appropriate function based on the file extension: YAML or XML.
      $func = 'patterns_load_' . substr($file->filename, strlen($file->name) + 1);

      // Load and save pattern.
      if (!($pattern = $func($file->uri))) {
        $messages['errors'][] = $file->filename;
        continue;
      }
      
      if (!patterns_validate_pattern($pattern)) {
        $messages['invalid'][] = $file->filename;
        continue;
      }   

      // If everything was fine save it to the database
      $result = patterns_db_save_pattern($pattern, $file->uri, $file->name);
      
      if ($result && !in_array($file->name, $existing)) {
        $messages['found'][] = $file->name;
        continue;
      }
      
      if (!$result && !in_array($file->name, $existing)) {
        $messages['error_found'][] = $file->name;
        continue;
      }
      
      if ($result && in_array($file->name, $existing)) {
        $messages['updated'][] = $file->name;
        continue;
      }
      
      if (!$result && in_array($file->name, $existing)) {
        $messages['error_updated'][] = $file->name;
        continue;
      }
      
    }
  }
  
  variable_set('patterns_loaded', time());
  if ($verbose) {
    _patterns_io_analyze_scandir_messages($messages);
  }
  return $messages;
}

/**
 * Analyzes the result of the patterns directories scan and 
 * displays relevant Drupal messages to the user.  
 * 
 * @see patterns_io_scan_directories
 * 
 */
function _patterns_io_analyze_scandir_messages($messages = NULL) {
  if (is_null($messages)) return;
  
  if (!empty($messages['found'])) {
    drupal_set_message(t('New patterns were found and added to the database: ') . '<br/>' . implode('<br/>', $messages['found']) . '<br/><br/>');
  }
  if (!empty($messages['updated'])) {
    drupal_set_message(t('The following patterns were updated to the newest version found in the file system: ') . '<br/>' . implode('<br/>', $messages['updated']) . '<br/><br/>');
  }
  if (!empty($messages['invalid'])) {
    drupal_set_message(t('Some invalid patterns were found: ') . '<br/>' . implode('<br/>', $messages['invalid']) . '<br/><br/>', 'warning');
  }
  if (!empty($messages['skipped'])) {
    drupal_set_message(t('Patterns that are already enabled are not updated against changes in the file systems. Please verify the following ones: ') . '<br/>' . implode('<br/>', $messages['skipped']) . '<br/><br/>', 'warning');
  }
  if (!empty($messages['permission'])) {
    drupal_set_message(t('Some pattern files could not be open for reading. Please verify the permission of: ') . '<br/>' . implode('<br/>', $messages['permission']) . '<br/><br/>', 'warning');
  }
  if (!empty($messages['error_found'])) {
    drupal_set_message(t('New patterns were found, but could not be saved in the database: ') . '<br/>' . implode('<br/>', $messages['error_found']) . '<br/><br/>', 'warning');
  }
  if (!empty($messages['error_updated'])) {
    drupal_set_message(t('A newer version of the following pattern was found, but could not be saved in the database: ') . '<br/>' . implode('<br/>', $messages['error_found']) . '<br/><br/>', 'warning');
  }
  
}

/**
 * Loads all the available patterns from the database.
 * It also check against the file systems, and, if the 
 * patterns global configuration options allow it, 
 * updates the database with the latest modifications. 
 * Notice: enabled patterns will not be updated anyway.
 *
 * @param $reset if TRUE, it always updates the patterns in
 *  the database with the file system. 
 *
 * @return array The array of available patterns.
 */
function patterns_get_patterns($reset = TRUE, $verbose = TRUE) {

  if ($reset || !variable_get('patterns_loaded', FALSE)) {
    // Updates the patterns in the database   
   patterns_io_scan_directories($verbose); 
  }
  
  // Get all the patterns from the database
  $result = patterns_db_get_patterns();

  $messages = array();
  $patterns = array();
  $patterns[PATTERNS_STATUS_OK] = array();
  $patterns[PATTERNS_STATUS_REMOVED] = array();


  foreach ($result as $pattern) {
    
    // Skip pattern if its name is missing.
    if (!isset($pattern->name)) {
      continue;
    }
    
    // Skip pattern if its file is missing.  
    if (!is_file($pattern->file)) {
      $messages[] = patterns_utils_toString($pattern);
    }

    $status = $pattern->status;
    if (in_array($status, array(PATTERNS_STATUS_OK, PATTERNS_STATUS_ENABLED, PATTERNS_STATUS_INVALID))) {
      $status = PATTERNS_STATUS_OK;
    }
    else {
      $status = PATTERNS_STATUS_REMOVED;
    }

    $patterns[$status][$pattern->pid] = $pattern;
    $data = unserialize($pattern->pattern);
    $patterns[$status][$pattern->pid]->pattern = $data;
    $patterns[$status][$pattern->pid]->info = $data['info'];

  }

  if (!empty($messages)) {
    drupal_set_message(t("The following patterns were found in the database, but not in the file system:") . '<br/>' . implode('<br/>', $messages) . '<br/>', 'warning');
  }

  return $patterns;
}

/**
 * Prints an HTML formatted list of patterns.
 */
function patterns_io_get_patterns_service($type = 'ajax') {

  if ($type == 'ajax') {
    $patterns = patterns_get_patterns();
    $output = '<div id="all_patterns_div"><strong>last update: </strong>' . date(DATE_RFC822);
    $output .= theme('patterns_list', $patterns);
    $output .= '</div>';

    print $output;
  }

}

/**
 * Checks if a .htaccess file exists to prevent downloads of pattern files.
 */
function _patterns_io_check_htaccess() {

  $path = patterns_config_get_files_dir();

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
 * Checks if the patterns directory exist and is writable.
 */
function _patterns_io_is_patterns_dir_ready($patterns_files_dir = '', $flag = FILE_MODIFY_PERMISSIONS) {
 
  // TODO: keep pattern_files_dir in a separate location. Warning: defining a constant creates an error.
  if (empty($patterns_files_dir)) {
    $patterns_files_dir = patterns_config_get_files_dir(); // TODO: move this out for performance
  }
  return (file_prepare_directory($patterns_files_dir, $flag)) ? TRUE : FALSE;
}


/**
 * Saves a pattern.
 *
 * Replaces the pattern if it already exists. Produces error messages if the
 * pattern could not be saved.
 *
 * @param string $content The content of the pattern to be saved.
 * @param string $name The name of the pattern file.
 * @param string $format
 *   (optional) The format of the pattern. Defaults to 'yaml'.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 *
 * @TODO: Move $format as an element of pattern, a column in the DB.
 *
 */
function patterns_io_save_pattern($content = NULL, $name = NULL, $format = 'yaml', $dir = NULL) {

  if (is_null($name)) {
    drupal_set_message(t('Cannot save pattern with \'NULL\' identifier.'), 'error');
    return FALSE;
  }
  if (!patterns_validate_pattern($content)) {
    drupal_set_message(t("Pattern '%name' could not be saved. Make sure edited code is well-formed.", array('%name' => $name)), 'error');
    return FALSE;
  }
  if (is_null($dir)) {
    $dir = patterns_config_get_files_dir();
  }
  if (!file_prepare_directory($dir, FILE_CREATE_DIRECTORY)) {
    drupal_set_message(t('Error: the pattern is not writable. Please check the file system permissions.'), 'error');
    return FALSE;
  }
  $path =  $dir . '/' . $name . '.' . $format;
  if (!file_unmanaged_save_data($content, $path, FILE_EXISTS_REPLACE)) {
    drupal_set_message(t('Error: file could not be saved.'), 'error');
    return FALSE;
  }

  // Load and save pattern.
  $load_func = 'patterns_load_' . $format;
  $pattern = $load_func($path);
  if (!$pattern) {
    drupal_set_message(t("Pattern '%name' could not be saved into the database. Make sure edited code is well-formed.", array('%name' => $name)), 'error');
    return FALSE;
  }
  
  patterns_db_save_pattern($pattern, $path, $name);
  drupal_set_message(t('Pattern %name was saved in %path.', array('%name' => $name,
                                                                  '%path' => $path,
  )));

  return TRUE;
}

function patterns_io_remove_pattern($path) {
  // @TODO
}
