<?php
/*
 * @file
 * Configuration file.
 *
 * Permissions, Help, Requirements. Settings form. Path to directories.
 *
 */

/**
 * Implements hook_permission().
 */
function patterns_permission() {
  return array(
    'administer patterns' => array(
      'title' => t('administer patterns'),
    ),
    // TODO: More detailed set of permissions.
  );
}


/**
 * Implements hook_help().
 */
function patterns_help($section, $arg = NULL) {
  $output = '';
  switch ($section) {
    case 'admin/patterns':
      $output = t('Looking for patterns files under the following locations:') . ' ' . theme('item_list', array('items' => patterns_config_get_paths()));
      break;
  }
  return $output;
}

/**
 * Display the pattern settings form.
 * @TODO proper doc.
 */
function patterns_config_settings($form, &$form_state) {
  
  $form['patterns_save_file'] = array(
    '#type' => 'textfield',
    '#title' => t('Default Directory for Patterns Files'),
    '#description' => t('By default, patterns files will be saved and loaded from here.'),
    '#default_value' => variable_get('patterns_save_file', 'sites/all/patterns/'),
  );
  
  $form['patterns_default_running_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Default Running Mode'),
    '#description' => t('The default running mode when running a pattern.'),
    '#options' => array('batch' => 'Batch', 'php' => 'PHP'),
    '#default_value' => variable_get('patterns_default_running_mode', 'batch'),
  );
  
  $form['patterns_form_helper'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Patterns Form Helper'),
    '#description' => t('When enabled, patterns form helper will dump $form_id and $form_state variables at the bottom of each page. Dump will always contain values from the latest form submission. This may be very helpful while writing your own patterns.'),
    '#default_value' => variable_get('patterns_form_helper', FALSE),
  );
  
  $form['patterns_update_db_from_fs'] = array(
    '#type' => 'checkbox',
    '#title' => t('Sync database and file system'),
    '#description' => t('When enabled, patterns in the database will be automatically updated to the latest version found in the file system. If the pattern was already been enabled, it just display a warning, but it does not execute the update.'),
    '#default_value' => variable_get('patterns_update_db_from_fs', TRUE),
  );
  
  $form['patterns_default_include_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Included patterns'),
    '#description' => t('Default behavior with included patterns.'),
    '#options' => array(PATTERNS_INCLUDE_NEVER => PATTERNS_INCLUDE_NEVER_TEXT,
                   PATTERNS_INCLUDE_FIRSTRUN => PATTERNS_INCLUDE_FIRSTRUN_TEXT,
                   PATTERNS_INCLUDE_UPDATE => PATTERNS_INCLUDE_UPDATE_TEXT,
                   PATTERNS_INCLUDE_FIRSTRUN_OR_UPDATE => PATTERNS_INCLUDE_FIRSTRUN_OR_UPDATE_TEXT,
                   PATTERNS_INCLUDE_ATTACHED => PATTERNS_INCLUDE_ATTACHED_TEXT,
                   PATTERNS_INCLUDE_ALWAYS => PATTERNS_INCLUDE_ALWAYS_TEXT,
  ),
    '#default_value' => patterns_get_include_mode(),
  );
  
  /* TODO: Outdated, disabled for now.
  $form['patterns_allow_publish'] = array(
    '#type' => 'checkbox',
    '#title' => t('Share your patterns'),
    '#description' => t('When enabled, you will be able to "publish" selected patterns and make them available to other patterns users on the following URL: %url.', array('%url' => url(NULL, array('absolute' => TRUE)) . (variable_get('clean_url', 0) ? '' : '?q=') . 'patterns.xml')),
    '#default_value' => variable_get('patterns_allow_publish', FALSE),
  );
   */
  
  // Reload patterns while we are here and ensure the lists are up to date.
  //patterns_get_patterns(TRUE);

  return system_settings_form($form);
}


/**
 * Implements hook_patterns_components().
 *
 * Tell the pattern engine where the components are.
 */
function patterns_patterns_components() {
  return drupal_get_path('module', 'patterns') . '/components/';
}

/**
 * Return the path to the directory containing the patterns files (imported  or
 * executed ones).
 *
 * We do not use directly variable_get because it had some issues with the batch
 *
 */
function patterns_config_get_files_dir() {
  return drupal_realpath('public://') . "/patterns/";
}

/**
 * Return a list of paths that will be scanned for pattern files.
 */
function patterns_config_get_paths() {
  // Why was this global?
  // global $profile;
  //$base_path = $_SERVER['DOCUMENT_ROOT'] . base_path();

  $patterns_paths = array(
    patterns_config_get_files_dir(), // must be first, in order not lose changes
    variable_get('patterns_save_file', 'sites/all/patterns/'),
    drupal_get_path('module', 'patterns') . '/patterns/',
  );

  $profile = variable_get('install_profile', 'default');
  if (isset($profile)) {
    $patterns_paths[] = 'profiles/' . $profile . '/patterns';
  }

  // Allow any module to include patterns too.
  foreach (module_invoke_all('patterns_directory') as $path) {
    if (is_dir($path)) {
      $patterns_paths[] = $path . '/patterns';
    }
  }

  return $patterns_paths;
}


/**
 * Return file extensions supported by the patterns module.
 *
 * @return array
 *   Array of supported file types.
 *
 * @todo Convert this into pluggable system.
 */
function patterns_config_get_file_types() {
  $result = array('xml', 'php');
  if (file_exists(libraries_get_path('spyc') . '/spyc.php')) {
    $result[] = 'yaml';
  }
  return $result;
}
