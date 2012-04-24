<?php
/*
 * TODO: Module enabling/disabling is done using a workaround because
 *       the form submitting way does not work for some reason
 * TODO: Only set a theme default if 'default' is 1.
 * @file
 * Patterns component for system related operations.
 */
function system_patterns($data) {

  $files = array('modules/system/system.admin.inc');

  $actions['modules'] = array('descr' => t('Enable/Disable Modules'),
    PATTERNS_CREATE => array(),
    PATTERNS_DELETE => array(),
    PATTERNS_FILES  => $files,
  );

  // Drupal7.x changed the theme setting form, so we use a custom one
  $actions['theme'] = array('descr' => t('Enable (and set default)/Disable Themes'),
    PATTERNS_CREATE => array('set_active_theme'),
    PATTERNS_DELETE => array('set_active_theme'),
    PATTERNS_FILES  => $files,
  ); // TODO: Don't run through form.

  // Determine necessary forms for action 'form' based on the data.
  $forms = array();
  if ($data) {
    if ($data['tag'] == 'form' && isset($data['form_id'])) {
      $forms[] = $data['form_id'];
    }
  }

  $actions['form'] = array('descr' => t('Submit Custom Forms'),
    PATTERNS_CREATE => $forms,
    PATTERNS_FILES  => $files,
  );

  $actions['call_php_func'] = array('descr' => t('Call PHP Functions'),
    PATTERNS_CREATE => array('call_php_func_form'),
  ); // TODO: Don't run through form.

  $actions['variables'] = array('descr'  => t('Set/Modify/Delete System Variables'),
    PATTERNS_CREATE => array(),
    PATTERNS_MODIFY => array(), // TODO: ??
    PATTERNS_DELETE => array(),
    PATTERNS_EXPORT => array(PATTERNS_EXPORT_ALL => 'system_patterns_export_all_variables', ),
  );

//  $actions['variable'] = $actions['variables']; // Alias, not available any more

 /* $actions['date'] = array('descr' => t('Create/edit/delete types of date and time'),
    PATTERNS_CREATE => array('system_configure_date_formats_form','system_add_date_format_type_form'),
    PATTERNS_MODIFY => array('system_configure_date_formats_form','system_date_time_settings'),
    PATTERNS_DELETE => array('system_delete_date_format_type_form'),
    PATTERNS_FILES =>$files
  );*/

  return $actions;

}

function system_patterns_actions($data) {
  return array(
        'system_modules' => t('System: Enable or disable module'),
        'set_active_theme' => t('System: Set active theme'),
        'form' => t('System: Execute form'),
        'call_php_func_form' => t('Call PHP function: Call function'),
        'variables' => t('System: Set or delete variables'),
        'variable' => t('System: Set or delete variable'),
/*        'system_add_date_format_type_form' => t('Create a type of date and time'),
        'system_date_time_settings' => t('Edit the types of date and time'),
        'system_delete_date_format_type_form' => t('Delete a type of date and time'),
        'system_configure_date_formats_form' => t('Create/edit formats of date and time'),*/
  );
}

function system_patterns_export_all_variables($args = NULL, &$result = NULL) {
  // Taken from the Devel module.
  $query = db_select('variable', 'v')->extend('TableSort');
  $query->fields('v', array('name', 'value'));
  $qresult = $query->execute();

  $actions = array(PATTERNS_CREATE => array('tag' => 'variables'));
  foreach ($qresult as $row) {
    $name = $row->name;
    $value = variable_get($name);
    $action = array('name' => $name, 'value' => $value);
    array_push($actions[PATTERNS_CREATE], $action);
  }
  $result = array($actions);
  return $result;
}

// Prepare data for processing
function system_patterns_prepare($action, $tag, &$data) {
  if ($tag == 'theme') {
    if ($data['value']) {
      $data['theme_default'] = $data['value'];
    }
  }
  elseif ($tag == 'modules') {
    // Make a <modules>modulename</modules> style tag work
    if (is_string($data) || (isset($data['value']) && !isset($data[0]))) {
      $data = array($data);
    }

    // Ensure proper data format for cases when <modules> tag contains
    // only single <module> tag.
    if (!empty($data['module']) && is_string($data['module'])) {
      $data[0]['value'] = $data['module'];
      unset($data['module']);
    }

    foreach ($data as &$item) {
      // Ensure that modules with tags like <module>modulename</module>
      // are represented as an array instead of a string
      if (is_string($item)) {
        $item = array('value' => $item);
      }
    }

    // proccess alias for delete
    if (isset($data['disable']) && !isset($data['delete'])) {
      $data['delete'] = $data['disable'];
    }
    unset($data['disable']);
  }/*
    elseif($tag=='date'){
    if(isset($data['format'])){
      $data['date_format']=$data['format'];
      unset($data['format']);
    }
    if(isset($data['type'])){
      if(!isset($data['machine_name'])){
        $data['machine_name']=str_replace(' ', '-', $data['type']);
      }
      $data['date_type']=$data['type'];
      unset($data['type']);
    }
  if($action==PATTERNS_MODIFY){
    if(isset($data['date_type'])&&isset($data['date_format'])){
      $new='date_format_'.$data['date_type'];
      $data[$new]=$data['date_format'];
    }
  }
  }*/
  elseif ($tag == 'form') {
  }
  else {
    switch ($tag) {
      case 'variable':
        // Turn <variable>value</variable> style tag function the same as <variables>
        $tag = 'variables';
        $data = array($data);
      case 'variables':
        // Make a <variables>modulename</variables> style tag work

        if (!$data[0]) {
          if ($data['variable'] && isset($data['variable'][0])) {
            $data = $data['variable'];
            unset($data['variable']);
          }
          elseif ($data['variable'] && isset($data['variable']['value'])) {
            $data[0] = $data['variable'];
            unset($data['variable']);
          }
          else {
            $temp = $data;
            $data[0] = $temp;
            unset($data['id'], $data['value'], $data['delete']);
          }
        }

        for ($i = 0, $total = count($data); $i < $total; $i++) {
          $item = &$data[$i];

          if (!isset($item['value']) && (!isset($item['delete']) || !$item['delete'])) {
            $value = $item;
            unset($value['name']);
            $item = array('name' => $item['name'], 'value' => $value);
          }

        }

        break;
    }
  }
  // TODO: other tags

  return patterns_results();
}

// Validate the values for an action before running the pattern.
function system_patterns_validate($action, $tag, &$data) {
  $status = PATTERNS_SUCCESS;
  $msg = '';

  if ($tag == 'theme') {
    // TODO: do this the Drupal7.x way
    $themes = system_rebuild_theme_data();
    if (!array_key_exists($data['theme_default'], $themes)) {
      $status = PATTERNS_ERR;
      $msg = t('%theme is not a valid theme.', array('%theme' => $data['theme_default']));
    }
  }
  elseif ($tag == 'modules') {
    if (!isset($modules_info) || !is_array($modules_info)) {
      $modules_info = system_rebuild_module_data(); // list of available modules.
    }

    $modules = module_list(); // List of enabled modules.
    $delete = $action === PATTERNS_DELETE;
    for ($i = 0; isset($data[$i]) && ($item = $data[$i]); $i++) {
      $module = $item['value'];

      // Ensure a module can be disabled safely.
      if ($delete) { // TODO: Move this out of the loop?
        if (array_key_exists($module, $modules_info)) { // Module is available.
          if (array_key_exists($module, $modules)) { // Module is enabled.
            /*
             * TODO: no more dependents field in Drupal7.x
             foreach ((array)$modules_info[$module]->info['dependents'] as $dependent) { // Dependency check.
             if (array_key_exists($dependent, $modules)) {
             $remove[] = $i;
             drupal_set_message(t('Warning: Could not disable %module because other modules depend on it.', array('%module' => $module)), "warning");
             break;
             }
             }
             */
          }
          else {
            $remove[] = $i;
            $status = PATTERNS_WARN;
            $msg .= t('Warning: Did not disable %module because it is already disabled.', array('%module' => $module)) . '<br/>';
          }
        }
        else {
          $remove[] = $i;
          $status = PATTERNS_WARN;
          $msg .= t('Warning: Could not disable %module because it is missing.', array('%module' => $module)) . '<br/>';
        }
      }
      // Ensure a module and all of its dependencies exist.
      else {
        if (!array_key_exists($module, $modules)) { // Module not yet enabled.
          if (!array_key_exists($module, $modules_info)) { // Module does not exist.
            $required[] = $module;
          }
          else {
            foreach ((array) $modules_info[$module]->info['dependencies'] as $dependency) {
              if (!array_key_exists($dependency, $modules) && !array_key_exists($dependency, $modules_info)) {
                $required[] = $dependency;
              }
            }
          }

          if (!empty($required)) {
            $status = PATTERNS_ERR;
            $msg .= t('%module can not be installed because the module or its dependencies are missing. Please download them and try again.', array('%module' => $module)) .
                    t('!title%dependencies', array('!title' => '<br/><b>' . t('Missing module(s): ') . '</b>', '%dependencies' => implode(', ', $required)));
          }
        }
        else {
          $status = PATTERNS_WARN;
          $msg .= t('Warning: Did not enable %module because it is already enabled.', array('%module' => $module)) . '<br/>';
        }

      }
    }
    if (!empty($remove)) {
      $result = array();
      foreach ($data as $key => $item) {
        if (!in_array($key, $remove)) {
          $result[] = $item;
        }
      }
      $data = $result;
    }
  }
  elseif ($tag == 'call_php_func') {
    if (empty($data['function'])) {
      $status = PATTERNS_ERR;
      $msg = t("A function is required for this tag");
    }
    elseif (!empty($data['type']) && empty($data['module'])) {
      $status = PATTERNS_ERR;
      $msg = t("If you specify a type you must specify a module. See the documentation for module_load_include.");
    }
    elseif (empty($data['type']) && !empty($data['module'])) {
      $status = PATTERNS_ERR;
      $msg = t("If you specify a module you must specify a type. See the documentation for module_load_include.");
    }
    elseif (!empty($data['filepath']) && !file_exists($data['filepath'])) {
      $status = PATTERNS_ERR;
      $msg = t('The file that you specified does not exist: %file', array('%file' => $data['filepath']));
    }
  }
  elseif ($tag == 'form') {
    if (!isset($data['form_id'])) {
      $status = PATTERNS_ERR;
      $msg = t('"form_id" is missing.');
    }
    // Attempt to load required include file from menu.
    list($menu, $masks) = menu_router_build();
    foreach ($menu as $item) {
      if (isset($item['page arguments'][0]) && $item['page arguments'][0] == $data['form_id']
          && !empty($item['include file'])) {
        $data['include'] = $item['include file'];
        break;
      }
    }

    if (!empty($data['include']) && is_file($data['include'])) {
      require_once $data['include'];
    }
    elseif (is_array($data['include'])) {
      $data['include'] = drupal_get_path('module', $data['include']['module']) . '/' . $data['include']['file'];
    }

    if (!function_exists($data['form_id'])) {
      if (!empty($data['module']) && is_string($data['module'])) {
        $modules = module_list();
        if (in_array($data['module'], $modules)) {
          // try most common include file names
          module_load_include('inc', $data['module']);
          module_load_include('inc', $data['module'], $data['module'] . '.admin');
          module_load_include('inc', $data['module'], $data['module'] . '.page');
        }
      }
    }
    if (!function_exists($data['form_id'])) {
      $status = PATTERNS_ERR;
      $msg = t("Couldn't load the form %form. Check if all required modules are enabled and try to define 'include' or 'module' for this action.", array('%form' => $data['form_id']));
    }
  }
  // TODO: other tags

  return patterns_results($status, $msg);
}

// Return which callback functions to actually use.
function system_patterns_callbacks($action, $tag, &$data) {
  if ($tag == 'modules') { // Custom functions will do the work.
    $result = array('modules_execute');
  }
  elseif ($tag == 'variables') { // Custom functions will do the work.
    $result = array('variables_execute');
  }
  else { // Just regular forms.
    $data['tag'] = 'form';
    $desc = system_patterns($data);
    unset($data['tag']);
    $result = $desc[$tag][$action];
  }
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Prepare for valid processing of this type of component
function system_patterns_build($action, $form_id, &$data = NULL, &$a) {
  $status = PATTERNS_SUCCESS;
  $msg = '';
  $result = NULL;

  if ($form_id == 'set_active_theme') {
    $data['op'] = t('Save configuration'); // TODO: ??
    //module_load_include('inc', 'system', 'system.admin');
    // $result = $data;
  }
  elseif ($form_id == 'call_php_func_form') {
    $data['type'] = empty($data['type']) ? '' : $data['type'];
    $data['module'] = empty($data['module']) ? '' : $data['module'];
    $data['name'] = empty($data['name']) ? '' : $data['name'];
    $data['filepath'] = empty($data['filepath']) ? '' : $data['filepath'];

    if (empty($data['arguments'])) {
      $data['arguments'] = array();
    }
    elseif (!is_array($data['arguments'])) {
      $data['arguments'] = array($data['arguments']);
    }
    _call_php_func_include_files($data); // TODO: this works here?
    if (!is_callable($data['function'])) {
      $status = PATTERNS_ERR;
      $msg = t('The given function %func is not callable', array('%func' => $data['function']));
    }
    // $result = $data;
  } // TODO: other forms?
  else { // Running a user-provided form through tag 'form'.
    $form_state = $data['form_state_extras'];
    unset($data['form_state_extras']);
    unset($data['include']);
    $form_state['values'] = $data;
    $form_state['submitted'] = FALSE;
    if (!isset($form_state['storage'])) {
      $form_state['storage'] = NULL;
    }
    $result = $form_state;
  }
  return patterns_results($status, $msg, $result);
}

// Build a patterns actions and parameters
function system_patterns_params($action, $form_id, &$data = NULL, &$a) {
  $result = NULL;

  if ($form_id == 'set_active_theme') {
    $result = array($data['theme_default'], isset($data['default']) ? TRUE : FALSE,
      isset($data['admin']) ? TRUE : FALSE);
  }
  elseif ($form_id == 'call_php_func_form') {
    $result = array($data['function'], $data['type'], $data['module'], $data['name'], $data['filepath'], $data['arguments']);
  }
  // TODO: other forms?

  /*
  elseif($form_id=='system_delete_date_format_type_form'){
    $data['confirm']=1;
    $result=array($data['date_type']);
  }*/
  else { // Running a user-provided form through tag 'form'.
    if (isset($data['args'])) {
      $params = $data['args'];
      unset($data['args']);
      $result = params;
    }
  }
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Cleanup any global settings after the action runs
function system_patterns_cleanup($action, $tag, &$data) {
  if ($tag == 'modules') {
    menu_rebuild(); // TODO: only do this if there has been a module enabled/disabled
  }
  return patterns_results();
}

function modules_execute($action, &$data) {
  $enabled = $disabled = array();
  $delete = $action === PATTERNS_DELETE;
  for ($i = 0; $module = $data[$i]; $i++) {
    if ($delete) { // TODO: Move this out of loop?
      module_disable(array($module['value']), TRUE);
      $disabled[] = $module['value'];
    }
    else {
      if (!module_enable(array($module['value']), TRUE)) {
        // TODO: use proper logging?
        drupal_set_message(t('Warning: Could not enable %module because a dependency is missing.', array('%module' => $module)), "warning");
      }
      else {
        $enabled[] = $module['value'];
      }
    }
  }

  $msg =  ((count($enabled) > 0) ? t('Module(s) %vars enabled.', array('%vars' => implode(', ', $enabled))) : t('No modules have been enabled.')) . ' ' .
          ((count($disabled) > 0) ? t('Module(s) %vars disabled.', array('%vars' => implode(', ', $disabled))) : t('No modules have been disabled.'));
  return patterns_results(PATTERN_SUCCESS, $msg);
}

function variables_execute($action, &$data) {
  $names = array();
  for ($i = 0; $variable = $data[$i]; $i++) {
    if ($action === PATTERNS_DELETE) {
      variable_del($variable['name']);
    }
    else {
      $var = variable_get($variable['name'], NULL);
      if (is_array($var)) {
        // make sure we don't lose parts of the array that were not defined by pattern's action
        // TODO: is this a good practice?
        $var = array_merge($var, $variable['value']);
        variable_set($variable['name'], $var);
      }
      else {
        variable_set($variable['name'], $variable['value']);
      }
    }
    $names[] = $variable['name'];
  }

  $msg = t('Variable(s) %vars updated.', array('%vars' => implode(', ', $names)));
  return patterns_results(PATTERNS_SUCCESS, $msg);
}

function set_active_theme($form, $form_state, $theme_default, $default, $admin) {
  return array(
    '#theme' => array(
      'theme_default' => $theme_default,
      'default' => $default,
      'admin' => $admin
    ),
  );
}

function set_active_theme_submit($form, &$form_state) {
  $values = $form['#theme'];
  $theme = $values['theme_default'];
  $default = $values['default'];
  $admin = $values['admin'];
  patterns_system_theme_default($theme, $default, $admin);

}
// Modified version of system_theme_default
function patterns_system_theme_default($theme, $default, $admin) {
  // Get current list of themes.
  $themes = list_themes();

  // Check if the specified theme is one recognized by the system.
  if (!empty($themes[$theme])) {
    // Enable the theme if it is currently disabled.
    if (empty($themes[$theme]->status)) {
      // NOTE: this assumes sites/default/css exists.
      theme_enable(array($theme));
    }
    if ($default) {
      if ($admin) {
        // Set the admin theme.
        variable_set('admin_theme', $theme);
      }
      else {
        // Set the default theme.
        variable_set('theme_default', $theme);
      }
    }

    // Rebuild the menu. This duplicates the menu_rebuild() in theme_enable().
    // However, modules must know the current default theme in order to use
    // this information in hook_menu() or hook_menu_alter() implementations,
    // and doing the variable_set() before the theme_enable() could result
    // in a race condition where the theme is default but not enabled.
    menu_rebuild();
  }

}

function call_php_func_form($form, $form_state, $func, $type, $module, $name, $filepath, $args) {
  return array(
    '#call_php_func' => array(
      'function' => $func,
      'type' => $type,
      'module' => $module,
      'name' => $name,
      'filepath' => $filepath,
      'arguments' => $args,
      ),
    );
}

function call_php_func_form_submit($form, &$form_state) {
  $values = $form['#call_php_func'];
  _call_php_func_include_files($values);

  call_user_func_array($values['function'], $values['arguments']);
}

function _call_php_func_include_files($values) {
  if ($values['type'] && $values['module']) {
    module_load_include($values['type'], $values['module'], $values['name']);
  }
  elseif ($values['filepath']) {
    require_once($values['filepath']);
  }
}

