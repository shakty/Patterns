<?php


function patterns_menu_alter($items) {
	
	/* NOT YET 
	
	$items['admin/patterns/drush/enable'] = array(
    'title' => 'Enable Modules',
    'page callback' => 'patterns_drush_enable_modules_service',
    'page arguments' => array(4),
    'access arguments' => array('administer patterns'),
    'type' => MENU_CALLBACK
	);
	
	$items['admin/patterns/drush/download'] = array(
    'title' => 'Enable Modules',
    'page callback' => 'patterns_drush_enable_modules_service',
    'page arguments' => array(4),
    'access arguments' => array('administer patterns'),
    'type' => MENU_CALLBACK
	);
	
	$items['admin/patterns/drush/dlenable'] = array(
    'title' => 'Download and Enable Modules',
    'page callback' => 'patterns_drush_enable_modules_service',
    'page arguments' => array(TRUE),
    'access arguments' => array('administer patterns'),
    'type' => MENU_CALLBACK
	);
	*/
}

/**
 * 
 * Enter description here ...
 */
function patterns_drush_download_modules_service() {
	if (!isset($_POST['modules'])) {
    print t('Communication error.');
    exit;
  }
  $modules = $_POST['modules'];
  $out = patterns_drush_download_modules($modules);
  print $out;
}

/**
 * 
 * Enter description here ...
 */
function patterns_drush_enable_modules_service($dl = FALSE) {
	if (!isset($_POST['modules'])) {
    print t('Communication error.');
    exit;
  }
  // Auto donwload if not available locally
  // Not used now
	$dl = (isset($_POST['download'])) ? TRUE : FALSE;
  $dep = (isset($_POST['dependencies'])) ? TRUE : FALSE;
 
  $modules = $_POST['modules'];
  $out = patterns_drush_download_modules($modules, $dep);
  print $out;
}



/**
 * Downloads modules with the drush dl command
 * Only if the variable patterns_drush_path is set.
 * @param array $modules a list of modules to download
 * returns an array with the lines of the output of the command
 */
function patterns_drush_download_modules($modules) {
  if (variable_get('patterns_drush_path') !== '') {
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
  if ($resolve_dependencies) {
    $rd = '--resolve-dependencies';
  } 
  
  if (variable_get('patterns_drush_path') !== '') {
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