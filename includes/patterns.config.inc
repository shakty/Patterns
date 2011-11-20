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
      $output = t('Patterns will be looked for in files under the following locations:') . ' ' . theme('item_list', array('items' => patterns_config_get_paths()));
      break;
  }
  return $output;
}

/**
 * Display the pattern settings form.
 * @TODO proper doc.
 */
function patterns_config_settings($form, &$form_state) {
  $form['patterns_form_helper'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Patterns Form Helper'),
    '#description' => t('When enabled, patterns form helper will dump $form_id and $form_state variables at the bottom of each page. Dump will always contain values from the latest form submission. This may be very helpful while writing your own patterns.'),
    '#default_value' => variable_get('patterns_form_helper', FALSE),
  );
  $form['patterns_allow_publish'] = array(
    '#type' => 'checkbox',
    '#title' => t('Share your patterns'),
    '#description' => t('When enabled, you will be able to "publish" selected patterns and make them available to other patterns users on the following URL: %url.', array('%url' => url(NULL, array('absolute' => TRUE)) . (variable_get('clean_url', 0) ? '' : '?q=') . 'patterns.xml')),
    '#default_value' => variable_get('patterns_allow_publish', FALSE),
  );
  // Reload patterns while we are here and ensure the lists are up to date.
  patterns_get_patterns(TRUE);

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
 */
function patterns_config_get_files_dir() {
  return drupal_realpath('public://') . "/" . variable_get('patterns_save_file', 'patterns') . '/'; 
}

/**
 * Return a list of paths that will be scanned for patterns/.
 */
function patterns_config_get_paths() {
  global $profile;

  if (!isset($profile)) {
    $profile = variable_get('install_profile', 'default');
  }
  $patterns_paths = array(
    //conf_path() . '/patterns',
    drupal_get_path('module', 'patterns') . '/patterns/',
    //'profiles/' . $profile . '/patterns',
    //'sites/all/patterns',
  );

  // Allow any module to include patterns too.
  foreach (module_invoke_all('patterns_directory') as $path) {
    if (is_dir($path)) {
      $patterns_paths[] = $path . '/patterns';
    }
  }

  $path = patterns_config_get_files_dir();

  // Ste: was directory_check (included in prepared dir)
  if (file_prepare_directory($path)) {
    array_unshift($patterns_paths, $path);
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
