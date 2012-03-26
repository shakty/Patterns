<?php
/*
 * @file
 *
 * API for writing pattern files.
 *
 */


/**
 * Helper function to create the associative array of returned values from
 * an invoked function.
 *
 * @param mixed $status One of [PATTERNS_SUCCESS, PATTERNS_WARN, PATTERNS_ERR]. Required.
 * @param array $msg A message.
 * @param mixed $result Any kind of additional data.
 *
 * @return array An associative array of the input values with keys 'status', 'msg', 'result'.
 */
function patterns_results($status = PATTERNS_SUCCESS, $msg = NULL, $result = NULL) {
  if ($msg === NULL) {
    $msg = t('Execution successful');
  }
  return array('status' => $status,
                'msg' => $msg,
                'result' => $result,
              );
}

/**
 * Checks if a (supposed) array is actually correctly formatted as a patterns_results().
 *
 * @param mixed $results The array to check.
 *
 * @return bool TRUE if correct, FALSE otherwise.
 * @see patterns_results()
 */
function _patterns_is_patterns_results($results) {
  // 'status' is the only mandatory field.
  if (!isset($results['status'])) {
    return FALSE;
  }

  return TRUE;
}


/**
 * Returns an array of allowed actions, as defined in patterns.module.
 *
 * @return array Allowed actions.
 */
function patterns_actions() {
  return array(PATTERNS_CREATE => PATTERNS_CREATE,
               PATTERNS_MODIFY => PATTERNS_MODIFY,
               PATTERNS_DELETE => PATTERNS_DELETE,
               PATTERNS_INCLUDE => PATTERNS_INCLUDE,
              );
}


/**
 * Downloads modules with the drush dl command
 * Only if the variable patterns_drush_path is set.
 * @param array $modules a list of modules to download
 * returns an array with the lines of the output of the command
 */
function patterns_drush_download_modules($modules) {
  if(variable_get('patterns_drush_path') !== '') {
    $output = array('following was output by drush');
    $cmd = variable_get('patterns_drush_path');
    $cmd .= ' -r '. patterns_get_drupal_installation_path();
    $cmd .= ' dl '. implode(' ', $modules) .' -y';
    exec($cmd , $output);
    return $output;
  } else {
    return array('Drush path wasn\'t set');
  }
}

/**
 * Downloads modules with the drush dl command
 * Only if the variable patterns_drush_path is set.
 * @param array $modules a list of modules to enable
 * @param bool $resolve_dependencies enable downloading and enabling dependencies
 * returns an array with the lines of the output of the command
 */
function patterns_drush_enable_modules($modules, $resolve_dependencies = FALSE) {
  $rd = '';
  if($resolve_dependencies) {
    $rd = '--resolve-dependencies';
  } 
  
  if(variable_get('patterns_drush_path') !== '') {
    $output = array('following was output by drush');
    $cmd = variable_get('patterns_drush_path');
    $cmd .= ' -r '. patterns_get_drupal_installation_path();
    $cmd .= ' en '. $rd .' '. implode(' ', $modules) .' -y';
    exec($cmd , $output);
    return $output;
  } else {
    return array('Drush path wasn\'t set');
  }
}

/**
 * returns installation path of drupal
 * TODO: more robust version if possible
 */
function patterns_get_drupal_installation_path() {
  // TODO: Patterns or patterns?
  return realpath(drupal_get_path('module', 'Patterns') .'/../../../..') . '/';
}