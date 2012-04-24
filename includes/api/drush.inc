<?php

module_load_include('inc', 'api');

function patterns_menu_alter(&$items) {

  $items['admin/patterns/drush/enable/%'] = array(
    'title' => 'Enable Modules',
    'page callback' => 'patterns_drush_enable_modules_service',
    'page arguments' => array(4),
    'access arguments' => array('administer patterns'),
    'type' => MENU_CALLBACK
  );

  $items['admin/patterns/drush/download/%'] = array(
    'title' => 'Enable Modules',
    'page callback' => 'patterns_drush_download_modules_service',
    'page arguments' => array(4),
    'access arguments' => array('administer patterns'),
    'type' => MENU_CALLBACK
  );

  $items['admin/patterns/drush/dlenable/%'] = array(
    'title' => 'Download and Enable Modules',
    'page callback' => 'patterns_drush_dlenable_modules_service',
    'page arguments' => array(4),
    'access arguments' => array('administer patterns'),
    'type' => MENU_CALLBACK
  );
}

/**
 * Calls drush dl and returns output and success/error messages
 * @param string $module the module to download
 * @param bool $raw If set to true, function returns $form instead of html
 */
function patterns_drush_download_modules_service($module, $raw = FALSE) {
  $out = patterns_drush_download_modules(array($module));

  $status = $out['status'];
  if ($status == PATTERNS_SUCCESS) {
    drupal_set_message(t('Drush download ran without errors. Check the output if no problems occured.'));
  }
  else {
    drupal_set_message(t('An error occured in drush download: drush returned return code ' . $out['result'] . '.  See drush output'), 'error'); // TODO: proper t()
  }

  $form['patterns']['drush']['download'] = array(
        '#title'  => 'Download:',
        '#prefix' => '<div>',
        '#suffix' => '</div><br/>',
        '#markup' => t(implode('<br/>', $out['msg'])),
  );

  return !$raw ? drupal_render($form) : $form;
}

/**
 * Calls drush en and returns output and success/error messages
 * @param string $module the module to download
 * @param bool $raw If set to true, function returns $form instead of html
 */
function patterns_drush_enable_modules_service($module, $raw = FALSE) {
  $dep = TRUE;
  $out = patterns_drush_enable_modules(array($module), $dep);
  $status = $out['status'];
  if ($status == PATTERNS_SUCCESS) {
    drupal_set_message(t('Drush enable ran without errors. Check the output if no problems occured.'));
  }
  else {
    drupal_set_message(t('An error occured in drush enable: drush returned return code ' . $out['result'] . '.  See drush output'), 'error'); // TODO: proper t()
  }

  $form['patterns']['drush']['enable'] = array(
        '#title'  => 'Enable:',
        '#prefix' => '<div>',
        '#suffix' => '</div><br/>',
        '#markup' => t(implode('<br/>', $out['msg'])),
  );
  return !$raw ? drupal_render($form) : $form;
}

/**
 * Calls drush dl and drush en. Returns output and success/error messages
 * @param string $module the module to download and enable
 */
function patterns_drush_dlenable_modules_service($module) {
  $form = patterns_drush_download_modules_service($module, TRUE);
  $form[] = patterns_drush_enable_modules_service($module, TRUE);
  return drupal_render($form);
}


/**
 * Downloads modules with the drush dl command
 * Only if the variable patterns_drush_path is set.
 * @param array $modules a list of modules to download
 * returns an array with the lines of the output of the command
 */
function patterns_drush_download_modules($modules) {
  if (!patterns_drush_ready()) {
    return array('Drush path wasn\'t set');
  }

  $output = array('Drush download output: ');
  $cmd = variable_get('patterns_drush_path');
  $cmd .= ' -r ' . patterns_get_drupal_installation_path();
  $cmd .= ' dl ' . implode(' ', $modules) . ' -y';
  $cmd .= ' 2>&1'; // add error stream

  $return_var;
  exec($cmd , $output, $return_var);
  return patterns_results($return_var === 0 ? PATTERNS_SUCCESS : PATTERNS_ERR, $output, $return_var);
}

/**
 * Downloads modules with the drush dl command
 * Only if the variable patterns_drush_path is set.
 * @param array $modules a list of modules to enable
 * @param bool $resolve_dependencies enable downloading and enabling dependencies
 * returns an array with the lines of the output of the command
 */
function patterns_drush_enable_modules($modules, $resolve_dependencies = FALSE) {
  if (!patterns_drush_ready()) {
    return array('Drush path wasn\'t set');
  }

  $rd = '';
  if ($resolve_dependencies) {
    $rd = '--resolve-dependencies';
  }


  $output = array('Drush enable output:');
  $cmd = variable_get('patterns_drush_path');
  $cmd .= ' -r ' . patterns_get_drupal_installation_path();
  $cmd .= ' en ' . $rd . ' ' . implode(' ', $modules) . ' -y';
  $cmd .= ' 2>&1'; // add error stream
  $return_var; // ??
  exec($cmd , $output, $return_var);

  return patterns_results($return_var === 0 ? PATTERNS_SUCCESS : PATTERNS_ERR, $output, $return_var);

}

/**
 * returns installation path of drupal
 * TODO: more robust version if possible
 */
function patterns_get_drupal_installation_path() {
  // TODO: Patterns or patterns?
  return realpath(drupal_get_path('module', 'Patterns') . '/../../../..') . '/';
}

function patterns_drush_ready() {
  return (variable_get('patterns_drush_path') !== '') ? TRUE : FALSE;
}
