<?php
/*
 * @file
 * Configuration file.
 */

/**
 * Implements hook_permission().
 */
function patterns_permission() {
  return array(
    'administer patterns' => array(
      'title' => t('administer patterns'),
    ),
    // TODO: more detailed set of permissions
  );
}


/**
 * Implements hook_help().
 */
function patterns_help($section, $arg = NULL) {
  $output = '';
  switch ($section) {
    case 'admin/patterns':
      $output = t('Patterns will be looked for in files under the following locations:') . ' ' . theme('item_list', array('items' => patterns_paths()));
      break;
  }
  return $output;
}


/**
 * Display the pattern settings form.
 */
function patterns_settings($form, &$form_state) {
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
  // Reload patterns while we are here and ensure the lists are up to date
  patterns_get_patterns(TRUE);
  
  return system_settings_form($form);
}

/**
 * Implements hook_patterns_directory().
 *
 * Let us know about where the pattern files are at.
 */
function patterns_patterns_directory() {
  return drupal_get_path('module', 'patterns');
}

/**
 * Implements hook_patterns_components().
 *
 * Let us know about where the components are.
 */
function patterns_patterns_components() {
  return drupal_get_path('module', 'patterns') . '/components/';
}

/**
 * Implements hook_requirements().
 *
 * @param string $phase The phase in which hook_requirements is run (install|runtime).
 */
function patterns_requirements($phase) {
  $requirements = array();

  switch ($phase) {
    case 'runtime':
      $path = libraries_get_path('spyc') . '/spyc.php';

      if (!file_exists($path)) {
        $requirements['spyc'] = array(
          'title'       => t('Spyc library (YAML parser)'),
          'description' => t('Patterns module requires Spyc library for working with YAML patterns. To enable YAML support, download the !spyc package to obtain spyc.php. Create a directory for the library (e.g. sites/all/libraries/spyc/) and place spyc.php there.',
                              array('!spyc' => l(t('spyc'), 'http://code.google.com/p/spyc/', array('absolute' => TRUE)))),
          'severity'    => REQUIREMENT_WARNING,
          'value'       => t('Missing'),
        );
      }
      else {
        require_once($path);
        $requirements['spyc'] = array(
          'title'       => t('Spyc library (YAML parser)'),
          'severity'    => REQUIREMENT_OK,
          'value'       => _get_file_phpdoc_version($path),
        );
      }
      break;
  }
  return $requirements;
}


/**
 * Return a list of paths that will be scanned for patterns/
 */
function patterns_paths() {
  global $profile;

  if (!isset($profile)) {
    $profile = variable_get('install_profile', 'default');
  }

  // array of all the paths where we should look for patterns
  $patterns_paths = array(
    conf_path() . '/patterns',
    'profiles/' . $profile . '/patterns',
    'sites/all/patterns'
  );

  // allow any module to include patterns too
  foreach (module_invoke_all('patterns_directory') as $path) {
    if (is_dir($path)) {
      $patterns_paths[] = $path . '/patterns';
    }
  }


  // Ste:was
  // also prepend files folder if it's valid
  //$path = file_create_path(variable_get('patterns_save_xml', 'patterns'));
  //$path = "/sites/default/files/patterns";

  // real_path(public) returns the path to the files dir
  $path = drupal_realpath('public://') . '/' . variable_get('patterns_save_xml', 'patterns');

  //Ste: was directory_check (included in prepared dir)
  if (file_prepare_directory($path)) {
    array_unshift($patterns_paths, $path);
  }
  return $patterns_paths;
}


/**
 * Return file extensions supported by patterns module.
 *
 * @return array of supported file types
 *
 * @todo convert this into pluggable system
 */
function patterns_file_types() {
  $result = array('xml', 'php');
  if (file_exists(libraries_get_path('spyc') . '/spyc.php')) {
    $result[] = 'yaml';
  }
  return $result;
}