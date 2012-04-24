<?php
/**
 * @file
 * Patterns modules page theme functions.
 */

/**
 * Lists the modules used by a particular pattern.
 *
 * @TODO Doc.
 */
function patterns_modules_page($pid) {
  $pattern = patterns_utils_if_invalid_go_back($pid);

  drupal_set_title($pattern->title . ' ' . t('(Pattern Modules)'));

  $modules = isset($pattern->pattern['modules']) ? $pattern->pattern['modules'] : array();
  $modules = is_array($modules) ? $modules : array($modules);
  $modules_info = system_rebuild_module_data();
  $modules_list = module_list();

  $not_available = FALSE;
  $rows = array();
  // Get module name, whether its to be disabled or enabled,
  // whether the module is available or not, and whether it is
  // currently enabled or not
  foreach ($modules as $module) {
    $row = array();
    $module = is_array($module) ? $module['value'] : $module;
    $available = array_key_exists($module, $modules_info);
    $enabled = array_key_exists($module, $modules_list);

    $row[] = $module;
    $row[] = $available ? t('Yes') : '<span class="alert">' . t('No') . '</span>';
    $row[] = $enabled ? t('Yes') : '<span class="alert">' . t('No') . '</span>';

    if ($available && ! $enabled) {
      $row[] = l(t('Enable'), 'admin/patterns/drush/enable/' . $module);

    }
    elseif (!$available) {
      $row[] = l(t('Download and Enable'), 'admin/patterns/drush/dlenable/' . $module);
    }
    else {
      $row[] = '-';
    }

    $row[] = '<span id="action_results"></span>';



    $rows[] = array('data' => $row);

    if (!$available) {
      $not_available = TRUE;
    }
  }

  if ($not_available) {
    drupal_set_message(t('Some modules are not available, please download them before running this pattern.'), 'error');
  }
  else {
    drupal_set_message(t('All modules required by this module are available. Click !here to run this pattern.', array('!here' => l(t('here'), 'admin/patterns/enable/' . $pid))));
  }

  if (empty($rows)) {
    $rows = array(
      array(
        'data' => array(
          array(
            'data' => t('No extra modules are required to run this pattern.'),
            'colspan' => '5',
          ),
        ),
      ),
    );

  }

  $header = array(t('Name'), t('Available'), t('Enabled'), t('Actions'));
  return theme('table', array('header' => $header, 'rows' => $rows, 'caption' => t('Modules used for this pattern')));

}
