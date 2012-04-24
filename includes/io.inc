<?php
/**
 * @file
 * Functions related to input/output operations.
 */


/**
 * Menu callback, returns source code of the requested pattern
 * if the pattern is public.
 *
 * @param integer $pid
 *   The ID of the Pattern to be displayed.
 */
function patterns_io_get_pattern_service($pattern) {


  $pattern = patterns_utils_if_invalid_go_back($pattern);

  // Make sure pattern is public (published).
  if (!$pattern->public) {
    drupal_set_message(t('Pattern %name is not public.', array('%name' => $pattern->name)), 'error');
    drupal_goto('admin/patterns');
  }

  // TODO: handling correctly different formats
  $content_type = 'text/plain';

  drupal_add_http_header('Content-Type', $content_type . '; charset=utf-8');
  print file_get_contents($pattern->file);
  exit;
}

/**
 * Loads the Patterns handlers (component) from the filesystem,
 * if they are not already loaded.
 *
 * @param bool $reset (optional) If TRUE, always forces reloading
 *   the components from the file system. Defaults FALSE
 * @param bool $dryrun (optional) If TRUE, it does not actually load
 *   the components, but just returns the paths to each of them.
 *   Defaults FALSE
 *
 * @return Array $components Array containing the paths of the
 *   components files.
 */
function patterns_io_load_components($reset = FALSE, $dryrun = FALSE) {
  $components = &drupal_static(__FUNCTION__);

  if (isset($paths) && !$reset) {
    return $components;
  }

  $components = array();

  // Get list of directories to scan for components.
  $paths = module_invoke_all('patterns_components');

  foreach ($paths as $path) {
    foreach (file_scan_directory($path, '/\.inc$/') as $file) {
      $components[] = $file->uri;
      if ($dryrun) continue;
      require_once $file->uri;
    }
  }

  return $components;
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
  $patterns_paths = patterns_path_get_patterns_dirs();

  // Get valid file extensions.
  $mask = '/.\.(' . implode('|', patterns_parser_get_formats()) . ')$/';

  // Prepare list of files/folders to be excluded.
  // 'enabled' - Don't save enabled pattern backups.
  $no_mask = array('.', '..', 'CVS', '.svn', '.git', '.bzr', 'enabled');

  foreach ($patterns_paths as $path) {

    foreach (file_scan_directory($path, $mask, $no_mask) as $file) {
      $format = substr($file->filename, strlen($file->name) + 1);

      if (in_array($file->filename, $existing)) {
        if (!variable_get('patterns_update_db_from_fs', TRUE)) {
          continue;
        }

        if (!patterns_db_is_pattern_updated($file->filename)) {
          // the database is already updated to the most recent version
          continue;
        }

        if (!empty($enabled)) {
          if (in_array($file->filename, $enabled)) {
            $messages['skipped'][] = $file->filename;
            continue; // Skip updating enabled patterns.
          }
        }
      }

      if (!is_readable($file->uri)) {
        $messages['permission'][] = $file->filename;
        continue;
      }

      // Choose appropriate function based on the file extension.
      // Can be FALSE, if no parser is found.
      $load_function = patterns_parser_get_parser_function($format, PATTERNS_PARSER_LOAD);

      // Load and save pattern.
      if (!$load_function || !($pattern = $load_function($file->uri))) {
        $messages['errors'][] = $file->filename;
        continue;
      }

      if (!patterns_validate_pattern($pattern)) {
        $messages['invalid'][] = $file->filename;
        continue;
      }

      // If everything was fine save it to the database
      $result = patterns_db_save_pattern($pattern, $file->uri, $file->filename, $format);

      if ($result && !in_array($file->filename, $existing)) {
        $messages['found'][] = $file->filename;
        continue;
      }

      if (!$result && !in_array($file->filename, $existing)) {
        $messages['error_found'][] = $file->filename;
        continue;
      }

      if ($result && in_array($file->filename, $existing)) {
        $messages['updated'][] = $file->filename;
        continue;
      }

      if (!$result && in_array($file->filename, $existing)) {
        $messages['error_updated'][] = $file->filename;
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
 * @see patterns_io_scan_directories()
 *
 */
function _patterns_io_analyze_scandir_messages($messages = NULL) {
  if (is_null($messages)) return;

  // TODO: fix these to be proper t().
  if (!empty($messages['found'])) {
    drupal_set_message(t('New patterns were found and added to the database:') . ' <br/>' . implode('<br/>', $messages['found']) . '<br/><br/>');
  }
  if (!empty($messages['updated'])) {
    drupal_set_message(t('The following patterns were updated to the newest version found in the file system:') . ' <br/>' . implode('<br/>', $messages['updated']) . '<br/><br/>');
  }
  if (!empty($messages['errors'])) {
    drupal_set_message(t('A generic error occurred while trying to load the following files:') . ' <br/>' . implode('<br/>', $messages['errors']) . '<br/><br/>', 'warning');
  }
  if (!empty($messages['invalid'])) {
    drupal_set_message(t('Some invalid patterns were found:') . ' <br/>' . implode('<br/>', $messages['invalid']) . '<br/><br/>', 'warning');
  }
  if (!empty($messages['skipped'])) {
    drupal_set_message(t('Patterns that are already enabled are not updated against changes in the file system. Please verify the following ones:') . ' <br/>' . implode('<br/>', $messages['skipped']) . '<br/><br/>', 'warning');
  }
  if (!empty($messages['permission'])) {
    drupal_set_message(t('Some pattern files could not be open for reading. Please verify the permission of:') . ' <br/>' . implode('<br/>', $messages['permission']) . '<br/><br/>', 'warning');
  }
  if (!empty($messages['error_found'])) {
    drupal_set_message(t('New patterns were found, but could not be saved in the database:') . ' <br/>' . implode('<br/>', $messages['error_found']) . '<br/><br/>', 'warning');
  }
  if (!empty($messages['error_updated'])) {
    drupal_set_message(t('A newer version of the following pattern was found, but could not be saved in the database:') . ' <br/>' . implode('<br/>', $messages['error_found']) . '<br/><br/>', 'warning');
  }

}

/**
 * Loads all the available patterns from the database.
 * It also check against the file system, and, if the
 * patterns global configuration options allow it,
 * updates the database with the latest modifications.
 * Notice: enabled patterns will not be updated anyway.
 *
 * @param $reset if TRUE, it always updates the patterns in
 *  the database with the file system.
 * @param bool $verbose If TRUE, only public patterns
 *   are returned
 * @param bool $verbose If TRUE, verbose output is
 *   printed to screen
 *
 * @return array The array of available patterns.
 */
function _patterns_io_get_patterns($reset = TRUE, $verbose = TRUE, $public = FALSE) {

  if ($reset || !variable_get('patterns_loaded', FALSE)) {
    // Updates the patterns in the database
    patterns_io_scan_directories($verbose);
  }

  // Get all the patterns from the database
  $result = patterns_db_get_patterns();

  $messages = array();
  $patterns = array();
  $patterns[PATTERNS_STATUS_OK] = array();
  $patterns[PATTERNS_STATUS_TRASHED] = array();


  foreach ($result as $pattern) {

    // Skip pattern if its name is missing.
    if (!isset($pattern->name)) {
      continue;
    }

    if ($public && !$pattern->public) {
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
      $status = PATTERNS_STATUS_TRASHED;
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
 * Wrapper function for _patterns_io_get_patterns.
 * @see _patterns_io_get_patterns()
 *
 * @return Array Associative array of public patterns.
 */
function patterns_io_get_public_patterns($reset = TRUE, $verbose = TRUE) {
  return _patterns_io_get_patterns($reset, $verbose, TRUE);
}

/**
 * Wrapper function for _patterns_io_get_patterns.
 * @see _patterns_io_get_patterns()
 *
 * @return array The array of available patterns.
 */
function patterns_io_get_patterns($reset = TRUE, $verbose = TRUE) {
  return _patterns_io_get_patterns($reset, $verbose, FALSE);
}

/**
 * Prints an HTML formatted list of patterns.
 */
function patterns_io_get_patterns_service($type = 'ajax') {

  if ($type == 'ajax') {
    $patterns = patterns_io_get_patterns();
    $output = '<div id="all_patterns_div"><strong>' . t('last update:') . '</strong>' . date(DATE_RFC822);
    $output .= theme('patterns_list', $patterns);
    $output .= '</div>';

    print $output;
  }

}

/**
 * Checks if a .htaccess file exists to prevent downloads of pattern files.
 */
function _patterns_io_check_htaccess() {

  $path = patterns_path_get_files_dir();

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
    $patterns_files_dir = patterns_path_get_files_dir(); // TODO: move this out for performance
  }
  return (file_prepare_directory($patterns_files_dir, $flag)) ? TRUE : FALSE;
}


/**
 * Saves a pattern string or array into the database AND in the file system.
 *
 * Updates the same file to the newer version, but does not replace an existing
 * file (e.g. if we are moving a file to a new location).
 *
 * Produces error messages if the pattern could not be saved.
 *
 * @param mixed $content The content of the pattern to be saved. Can be a string the
 *  or an array represeting the pattern. In the latter case @param $original
 *  can be contain the string. If @param $original is missing the pattern is
 *  saved as a PHP array.
 * @param mixed $name The name of the pattern file.
 * @param mixed $format file format (notice: it is not the extension!)
 *   (optional) The format of the pattern. Defaults to PATTERNS_FORMAT_YAML.
 * @param mixed $dir the destination directory
 * @param mixed $original the string representation of the pattern. Optional.
 *
 * @return bool
 *   TRUE on success, FALSE otherwise.
 *
 * TODO: if content is array and original is null, dump the array into the
 *  correct original file
 *
 */
function patterns_io_save_pattern($content = NULL, $name = NULL, $format = PATTERNS_FORMAT_YAML, $dir = NULL, $original = NULL) {

  if (is_null($name)) {
    drupal_set_message(t('Cannot save pattern with \'NULL\' identifier.'), 'error');
    return FALSE;
  }

  if (!patterns_validate_pattern($content, $format)) {
    drupal_set_message(t("Pattern '%name' could not be saved. Make sure edited code is well-formed.", array('%name' => $name)), 'error');
    return FALSE;
  }

  if (is_null($dir)) {
    $dir = patterns_path_get_files_dir();
  }
  if (!file_prepare_directory($dir, FILE_CREATE_DIRECTORY)) {
    drupal_set_message(t('Error: the pattern is not writable. Please check the file system permissions.'), 'error');
    return FALSE;
  }

  // Check if the file has a valid extension
  // and in case add the format at the end
  if (!_patterns_io_file_has_valid_extension($name)) {
    $name = $name . '.' . $format;
  }

  $path_original = $dir . '/' . $name;

  if (is_null($original)) {
    if (is_array($content)) {
      $original = patterns_parser_dump($content, $format);
    }
    else {
      $original = $content;
    }
  }

  $path = file_unmanaged_save_data($original, $path_original, FILE_EXISTS_REPLACE);
  if (!$path) {
    drupal_set_message(t('An error occurred while saving the file to %path. A file with the same name exists.', array('%path' => $path_original)), 'error');
    return FALSE;
  }

  // Load and save pattern.
  $load_function = patterns_parser_get_parser_function($format, PATTERNS_PARSER_LOAD);
  if (!$load_function) {
    drupal_set_message(t('Could not find a parser for ', array('%name' => $name)), 'error'); // TODO: proper t()
    return FALSE;
  }

  $pattern = $load_function($path);
  if (!$pattern) {
    drupal_set_message(t("Pattern '%name' could not be saved into the database. Make sure edited code is well-formed.", array('%name' => $name)), 'error');
    patterns_io_remove_pattern_from_fs($path);
    return FALSE;
  }

  patterns_db_save_pattern($pattern, $path, $name, $format);
  $link = l($name, 'admin/patterns/edit/' . $name);
  drupal_set_message(t('Pattern !name was saved in %path.', array('!name' => $link, '%path' => $path, )));

  return TRUE;
}

/**
 * Removes a pattern both from the file system and the database.
 *
 * @param mixed $pattern_orig A pattern object, an array representing
 *  the pattern object, a numeric id or alphanumeric name of
 *  the pattern as it is in the database
 * @param bool $verbose (optional) If TRUE notify the users a failure.
 *   Default FALSE.
 *
 * @return Boolean TRUE, if removal from the file system and the
 *   database is successful
 */
function patterns_io_remove_pattern($pattern_orig, $verbose = TRUE) {

  // Retrieve
  $pattern = _patterns_db_get_pattern($pattern_orig);

  if (!$pattern) {
    if ($verbose) {
      drupal_set_message(t('Impossible to retrieve specified pattern: %pattern.', array('%pattern' => $pattern_orig)), 'error');
    }
    return FALSE;
  }

  if (!patterns_io_remove_pattern_from_fs($pattern->file, $verbose)) {
    return FALSE;
  }

  $result = patterns_db_remove_pattern($pattern->pid);

  if ($result) {
    if ($verbose) {
      drupal_set_message(t('Pattern %pattern was removed succesfully.', array('%pattern' => $pattern_orig)));
    }
  }

  return $result;
}

function patterns_io_remove_pattern_from_fs($path, $verbose = TRUE) {

  // Check permissions
  if (!is_writable($path)) {
    if ($verbose) {
      drupal_set_message(t('Do not have permission to delete file %pattern. Aborting.', array('%pattern' => $path)), 'error');
    }
    return FALSE;
  }

  // Delete from FS
  $result = unlink($path);
  if (!$result) {
    if ($verbose) {
      drupal_set_message(t('An error occurred while deleting  %pattern.', array('%pattern' => $path)), 'error');
    }
    return FALSE;
  }

  return TRUE;
}



/**
 * Checks whether the extension of the specified file name
 * matches one of the currently available parser formats.
 *
 *
 * @param mixed $file String representing a file name.
 * @param array $formats (optional) The array of valid formats,
 *   against which checking the file extension. If NULL, rebuilds
 *   the index of currently available formats.
 *
 * @return bool Boolean. TRUE, if the extension matches an available
 *   parser.
 */
function _patterns_io_file_has_valid_extension($file = NULL, $formats = NULL) {
  if (is_null($file)) return FALSE;
  if (is_null($formats)) {
    $formats = patterns_parser_get_formats();
  }

  $ext = pathinfo($file, PATHINFO_EXTENSION);
  return (in_array($ext, $formats)) ? TRUE : FALSE;
}


/**
 * Returns a list of uris to components files.
 *
 * Wrapper method for patterns_io_load_components($reset, TRUE);
 *
 * @param bool $reset (optional) If TRUE, always forces reloading
 *   the components from the file system. Defaults FALSE
 *
 * @see patterns_io_load_components()
 */
function patterns_io_list_components($reset = FALSE) {
  return patterns_io_load_components($reset, TRUE);
}

/**
 * Returns a list of all the directories containing
 * Patterns components.
 *
 * @param bool $reset (optional) If TRUE, forces to
 *   reload the directory index.
 */
function patterns_io_list_components_dirs($reset = FALSE) {
  $components_dirs = &drupal_static(__FUNCTION__);

  if (isset($components_dirs) && !$reset) {
    return $components_dirs;
  }

  // Get list of directories to scan for components.
  return module_invoke_all('patterns_components');
}

