<?php

/**
 * @file
 * Functions to install and uninstall modules during the pattern
 * execution.
 *
 */


/**
 * Installs required modules for executing the pattern,
 * if they are not already enabled.
 *
 * Returns an associative array containing information about
 * which modules have been actually installed.
 *
 * @param array $modules The list of modules name to install.
 *
 */
function patterns_install_modules(&$modules) {
  $result = array();
  $result['success'] = TRUE;
  $result['installed_modules'] = array();

  if (empty($modules)) {
    return $result;
  }

  $to_install = patterns_check_modules_to_install($modules);
  if (!empty($to_install['missing'])) {
    $result['success'] = FALSE;
    $result['error_message'] = t('Following required modules are missing: %modules', array('%modules' => implode(', ', $to_install['missing'])));
    $result['missing_modules'] = $to_install['missing'];
    return $result;
  }
  if (!empty($to_install['to_install'])) {
    return $result;
  }

  require_once DRUPAL_ROOT . '/' . 'includes/install.inc';
  $result['success'] = module_enable($to_install['to_install'], TRUE);
  system_rebuild_module_data();
  $result['installed_modules'] = $to_install['to_install'];

  return $result;
}

/**
 * Disable the specified modules.
 *
 * @param array $modules
 * @param boolean $deps if TRUE also all the modules dependent
 *   will be disabled as well.
 *
 */
function patterns_disable_modules(&$modules, $deps = TRUE) {
  $result = array('success' => TRUE);

  if (empty($modules)) {
    return $result;
  }

  require_once DRUPAL_ROOT . '/' . 'includes/install.inc';
  module_disable($modules, TRUE);
  system_rebuild_module_data();

  $result['disabled_modules'] = $modules;

  return $result;
}


/**
 * Checks a list of modules and returns information about their
 * status, whether they are installed, to install or missing.
 *
 * If @param $deps is TRUE, also dependencies are checked.
 *
 * @param array $modules Array of module names.
 * @param boolean $deps If TRUE dependecies are also checked.
 * @return array $result The associative array of information
 *   about the modules.
 */
function patterns_check_modules_to_install($modules, $deps = TRUE) {
  $result = array();
  $result['missing'] = array();
  $result['to_install'] = array();
  $result['installed'] = array();
  if (empty($modules)) {
    return $result;
  }

  // Get information about all currently available modules
  $modules_info = system_rebuild_module_data();
  $active_modules = module_list();

  foreach ($modules as $module) {
    // Useful check for the XML format
    $module = is_array($module) ? $module['value'] : $module;

    if (array_key_exists($module, $modules_info)) {
      // Is the module to install ?
      if (isset($active_modules[$module])) {
        array_push($result['installed'], $module);
      }
      else {
        array_push($result['to_install'], $module);
      }

      // Shall we check the dependecies as well ?
      if (!$deps) {
        continue;
      }
      foreach ($modules_info[$module]->info['dependencies'] as $dependency) {
        if (!array_key_exists($dependency, $modules_info)) {
          array_push($result['missing'], $dependency);
        }
        else {
          if (isset($active_modules[$dependency])) {
            array_push($result['installed'], $dependency);
          }
          else {
            array_push($result['to_install'], $dependency);
          }
        }
      }
    }
    else {
      array_push($result['missing'], $module);
    }
  }

  return $result;
}

