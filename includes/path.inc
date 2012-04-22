<?php
/*
 * @file
 * Configuration file.
 *
 * Permissions, Help, Requirements. Settings form. Path to directories.
 *
 */


/**
 * Return the path to the directory containing the patterns files (imported  or
 * executed ones).
 *
 * We do not use directly variable_get because it had some issues with the batch
 *
 */
function patterns_path_get_files_dir() {
  return drupal_realpath('public://') . "/patterns/";
}

/**
 * Return a list of paths that will be scanned for pattern files.
 */
function patterns_path_get_patterns_dirs() {

  $patterns_paths = array(
    patterns_path_get_files_dir(), // must be first, in order not lose changes
    variable_get('patterns_save_file', 'sites/all/patterns/'),
    drupal_get_path('module', 'patterns') . '/patterns/',
  );

  $profile = variable_get('install_profile', 'default');
  if (isset($profile)) {
    $patterns_paths[] = 'profiles/' . $profile . '/patterns';
  }

  // Allow any module to include patterns too.
  foreach (module_invoke_all('patterns_directory') as $path) {
    $path = (is_array($path)) ? $path : array($path);
    foreach ($path as $p) {
      if (is_dir($p) && is_readable($p)) {
        $patterns_paths[] = $p ;
      }
    }
  }

  return $patterns_paths;
}


/**
 * Return a list of paths that will be scanned for pattern files.
 */
function patterns_path_get_components_dirs() {

  $patterns_paths = array(
    patterns_path_get_files_dir(), // must be first, in order not lose changes
    variable_get('patterns_save_file', 'sites/all/patterns/'),
    drupal_get_path('module', 'patterns') . '/patterns/',
  );

  $profile = variable_get('install_profile', 'default');
  if (isset($profile)) {
    $patterns_paths[] = 'profiles/' . $profile . '/patterns';
  }

  // Allow any module to include patterns too.
  foreach (module_invoke_all('patterns_directory') as $path) {
    $path = (is_array($path)) ? $path : array($path);
    foreach ($paths as $p) {
      if (is_dir($p) && is_readable($p)) {
        $patterns_paths[] = $p ;
      }
    }
  }

  return $patterns_paths;
}
