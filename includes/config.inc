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
    'get patterns' => array(
      'title' => t('get patterns'),
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


      $dirs = patterns_path_get_patterns_dirs();

      foreach ($dirs as $key => $value) {
        $dirs[$key] = array(($key+1), $value);
      }
      
      $form['pd'] = array(
        '#type' => 'fieldset',
        '#title' => t('Patterns Directories'),
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
      );
      
      $form['pd']['dirs'] = array (
        '#markup' => theme('table', array('rows' => $dirs, 'attributes' => array('class' => 'patterns-list'))),
      );
      
      $form['cd'] = array(
        '#type' => 'fieldset',
        '#title' => t('Patterns Components Directories'),
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
      );
      
      $form['cd']['dirs'] = array (
        '#markup' => 'TODO',
      );
      
      $output = drupal_render($form);
      break;
  }
  return $output;
}

/**
 * Display the pattern settings form.
 * @TODO proper doc.
 */
function patterns_config_settings($form, &$form_state) {


  $form['exec'] = array(
    '#type' => 'fieldset',
    '#title' => t('File system options'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['exec']['patterns_default_running_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Default Running Mode'),
    '#description' => t('The default running mode when running a pattern.'),
    '#options' => array('batch' => t('Batch'), 'php' => t('PHP')),
    '#default_value' => variable_get('patterns_default_running_mode', 'batch'),
  );

  $form['exec']['patterns_default_include_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Included patterns'),
    '#description' => t('Default behavior with included patterns. This option can be overwritten inside the pattern code.'),
    '#options' => array(PATTERNS_INCLUDE_NEVER => t('Never run'),
                   PATTERNS_INCLUDE_FIRSTRUN => t('Only during the first run'),
                   PATTERNS_INCLUDE_UPDATE => t('Only if pattern was updated'),
                   PATTERNS_INCLUDE_FIRSTRUN_OR_UPDATE => t('If it is the first run or if the pattern was updated'),
                   PATTERNS_INCLUDE_ATTACHED => t('Only if the pattern code is attached (no lookup to database)'),
                   PATTERNS_INCLUDE_ALWAYS => t('Always'),
    ),
    '#default_value' => patterns_get_include_mode(),
  );

  $form['fs'] = array(
    '#type' => 'fieldset',
    '#title' => t('File system options'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['fs']['patterns_save_file'] = array(
    '#type' => 'textfield',
    '#title' => t('Default Directory for Patterns Files'),
    '#description' => t('By default, patterns files will be saved and loaded from here.'),
    '#default_value' => variable_get('patterns_save_file', 'sites/all/patterns/'),
  );
  
  $form['fs']['patterns_refresh_components'] = array(
    '#type' => 'checkbox',
    '#title' => t('Updates the patterns component at every page reload'),
    '#description' => t('If selected, at every page reload on \'admin/patterns\' the components will be updated. It increases loading time, but keeps the components database always up to date.'),
    '#default_value' => variable_get('patterns_refresh_components', TRUE),
  );

  $form['fs']['patterns_refresh_from_fs'] = array(
    '#type' => 'checkbox',
    '#title' => t('Scan for new patterns at every page reload'),
    '#description' => t('If selected, at every page reload on \'admin/patterns\' the directories will be scanned for new patterns. It increases loading time, but keeps the patterns database always up to date.'),
    '#default_value' => variable_get('patterns_refresh_from_fs', TRUE),
  );

  $form['fs']['patterns_update_db_from_fs'] = array(
    '#type' => 'checkbox',
    '#title' => t('Sync database and file system'),
    '#description' => t('When enabled, patterns in the database will be automatically updated to the latest version found in the file system. If the pattern was already been enabled, it just display a warning, but it does not execute the update.'),
    '#default_value' => variable_get('patterns_update_db_from_fs', TRUE),
  );
  
  $form['publish'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sharing options'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['publish']['patterns_allow_publish'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable sharing of patterns'),
    '#description' => t('If selected, patterns can be shared with others web sites.'),
    '#default_value' => variable_get('patterns_allow_publish', PATTERNS_ALLOW_PUBLISH_DEFAULT),
  );

  $form['other'] = array(
    '#type' => 'fieldset',
    '#title' => t('Other'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,

  );

  $form['other']['patterns_form_helper'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Patterns Form Helper'),
    '#description' => t('When enabled, patterns form helper will dump $form_id and $form_state variables at the bottom of each page. Dump will always contain values from the latest form submission. This may be very helpful while writing your own patterns.'),
    '#default_value' => variable_get('patterns_form_helper', FALSE),
  );

  return system_settings_form($form);
}