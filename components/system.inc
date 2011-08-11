<?php
/*
 * TODO: module enabling/disabling is done using a workaround because
 *       the form submitting way does not work for some reason
 * @file
 * Patterns component for system related operations.
 */
function system_patterns($op, $id = NULL, &$data = NULL) {
  static $modules_info;

  switch ($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('modules', 'form', 'variables', 'variable', 'theme', 'call_php_func');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'system_modules' => t('System: Enable or disable module'),
        'variables' => t('System: Set or delete variables'),
        'variable' => t('System: Set or delete variable'),
        'set_active_theme' => t('System: Set active theme'), // Drupal7.x removed the theme setting form, so we use a custom one
        'form' => t('System: Execute form'),
        'call_php_func_form' => t('Call PHP function: Call function'),
      );
    break;

//    // Return a summary of an action
//    case 'summary':
//      switch ($id) {
//        case 'system_modules':
//          $enable = $disable = array();
//          for($i=0;$item=$data[$i];$i++) {
//             if ($item[$i]['delete']) {
//               $disable[] = $data[$i]['value'];
//             }
//             else {
//               $enable[] = $data[$i]['value'];
//             }
//          }
//
//          $enable = implode(', ', $enable);
//          $disable = implode(', ', $disable);
//
//          return t('Enabling modules: %enable', array('%enable' => $enable)) .'<br />'. t('Disabling modules: %disable', array('%disable' => $disable));
//        break;
//        case 'variables':
//          for($i=0;$item=$data[$i];$i++) {
//             $variables[] = $item['name'];
//          }
//
//          return t('Setting variables: %vars', array('%vars' => $variables));
//        break;
//        case 'variable':
//          return t('Setting variable %var', array('%var' => $data['name']));
//        break;
//        case 'system_themes_form':
//          return t('Setting theme to %name', array('%name' => $data['value']));
//        break;
//        case 'form':
//          return t('Executing form: %name', array('%name' => $data['form_id']));
//        break;
//        case 'call_php_func_form':
//          return t('Calling PHP function %func', array('%pattern' => $data['func']));
//        break;
//      }
//    break;

    // Prepare data for processing
    case 'prepare':
      switch ($id) {
        case 'variable':
          // Turn <variable>value</variable> style tag function the same as <variables>
          $id = 'variables';
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

          for ($i=0, $total = count($data); $i < $total; $i++) {
            $item = &$data[$i];

            if (!isset($item['value']) && (!isset($item['delete']) || !$item['delete'])) {
              $value = $item;
              unset($value['name']);
              $item = array('name' => $item['name'], 'value' => $value);
            }

          }

        break;
        case 'modules':
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
        break;
        case 'theme':
            if ($data['value']) {
                $data['theme_default'] = $data['value'];
            }
        break;
        case 'form':
          // attempt to load required include file
          list($menu, $masks) = menu_router_build();
          foreach ($menu as $item) {
            if (isset($item['page arguments'][0]) && $item['page arguments'][0] == $data['form_id']) {
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
        break;
        case 'call_php_func_form':
        break;
      }
    break;

    // Pre validate actions
    case 'pre-validate':
      if ($id == 'modules') {
        if (!is_array($modules_info)) {
          $modules_info = system_rebuild_module_data(); // list of available modules
        }

        $modules = module_list(); // list of enabled modules
        for ($i = 0; isset($data[$i]) && ($item = $data[$i]); $i++) {
          $module = $item['value'];
          $delete = (!empty($data['delete']) || !empty($item['delete']));

          // Ensure a module can be disabled safely
          if ($delete) {
            if (array_key_exists($module, $modules_info)) { // Module is available
              if (array_key_exists($module, $modules)) { // Module is enabled
              /*
          	   * TODO: no more dependents field in Drupal7.x
                foreach ((array)$modules_info[$module]->info['dependents'] as $dependent) { // Dependency check
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
                drupal_set_message(t('Warning: Did not disable %module because it is already disabled.', array('%module' => $module)), "warning");
              }
            }
            else {
              $remove[] = $i;
              drupal_set_message(t('Warning: Could not disable %module because it is missing.', array('%module' => $module)), "warning");
            }
          }
          // Ensure a module and all of its dependencies exist
          else {
            if (!array_key_exists($module, $modules)) { // Module not yet enabled
              if (!array_key_exists($module, $modules_info)) { // Module does not exist
                $required[] = $module;
              }
              else {
                foreach ((array)$modules_info[$module]->info['dependencies'] as $dependency) {
                  if (!array_key_exists($dependency, $modules) && !array_key_exists($dependency, $modules_info)) {
                    $required[] = $dependency;
                  }
                }
              }

              if (!empty($required)) {
                return t('%module can not be installed because the module or its dependencies are missing. Please download them and try again.', array('%module' => $module)) .
                       t('!title%dependencies', array('!title' => '<br/><b>' . t('Missing module(s): ') . '</b>', '%dependencies' => implode(', ', $required)));
              }
            }
            else {
              drupal_set_message(t('Warning: Did not enable %module because it is already enabled.', array('%module' => $module)), "warning");
            }
            
          }
        }
        if (!empty($remove)) {
          $result = array();
          foreach ($data as $key => $item) {
            if (!in_array($key, $remove)) $result[$key] = $item;
          }
          $data = $result;
        }
      }
      elseif ($id == 'theme') {
        // TODO: do this the Drupal7.x way
        $themes = system_rebuild_theme_data();
        if (!array_key_exists($data['theme_default'], $themes)) {
            return t('%theme is not a valid theme.', array('%theme' => $data['theme_default']));
        }
      }
      elseif ($id == 'form') {
        if (!array_key_exists('form_id', $data)) {
            return t('"form_id" is missing.');
        }
        if (!function_exists($data['form_id'])) {
            return t("Couldn't load the form %form. Check if all required modules are enabled and try to define 'include' or 'module' for this action.", array('%form' => $data['form_id']));
        }
      }
      elseif ($id == 'call_php_func') {
        if (empty($data['function'])) {
          return t("A function is required for this tag");
        }
        elseif (!empty($data['type']) && empty($data['module'])) {
          return t("If you specify a type you must specify a module. See the documentation for module_load_include.");
        }
        elseif (empty($data['type']) && !empty($data['module'])) {
          return t("If you specify a module you must specify a type. See the documentation for module_load_include.");
        }
        elseif (!empty($data['filepath']) && !file_exists($data['filepath'])) {
          return t('The file that you specified does not exist: %file', array('%file' => $data['filepath']));
        }
      }
    break;

    // Return the form_id('s) for each action
    case 'form_id':
      if ($id == 'modules') {
        return 'system_modules';
      }
      elseif ($id == 'variables' || $id == 'variable') {
        return 'variables';
      }
      elseif ($id == 'theme') {
        return 'set_active_theme';
      }
      elseif ($id == 'form') {
        if (!empty($data['include']) && is_file($data['include'])) {
          require_once $data['include'];
        }
        elseif (!empty($data['module']) && is_string($data['module'])) {
          $modules = module_list();
          if (in_array($data['module'], $modules)) {
            // try most common include file names
            module_load_include('inc', $data['module']);
            module_load_include('inc', $data['module'], $data['module'] . '.admin');
            module_load_include('inc', $data['module'], $data['module'] . '.page');
          }
        }
        return $data['form_id'];
      }
      elseif ($id == 'call_php_func') {
        return 'call_php_func_form';
      }
    break;

    // Prepare for valid processing of this type of component
    case 'build':
      if ($id == 'system_modules') {

        $enabled = $disabled = array();
        $delete = (!empty($data['delete']) || !empty($item['delete']));
        for ($i = 0; $module = $data[$i]; $i++) {
          if ($delete) {
            module_disable(array($module['value']), TRUE);
            $disabled[] = $module['value'];
          }
          else {
            if (!module_enable(array($module['value']), TRUE)) {
              drupal_set_message(t('Warning: Could not enable %module because a dependency is missing.', array('%module' => $module)), "warning");
            }
            else {
              $enabled[] = $module['value'];
            }
          }

        }
 
        return ((count($enabled) > 0) ? t('Module(s) %vars enabled.', array('%vars' => implode(', ', $enabled))) : t('No modules have been enabled.')) . ' ' .
               ((count($disabled) > 0) ? t('Module(s) %vars disabled.', array('%vars' => implode(', ', $disabled))) : t('No modules have been disabled.')) ;
      }
      elseif ($id == 'variables') {
        $names = array();
        for ($i = 0; $variable = $data[$i]; $i++) {
          if ($variable['delete']) {
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

        return t('Variable(s) %vars updated.', array('%vars' => implode(', ', $names)));
      }
      elseif ($id == 'set_active_theme') {
        $data['op'] = t('Save configuration');
        //module_load_include('inc', 'system', 'system.admin');
        return $data;
      }
      elseif ($id == $data['form_id']) {
        $form_state = $data['form_state_extras'];
        unset($data['form_state_extras']);
        unset($data['include']);
        $form_state['values'] = $data;
        $form_state['submitted'] = FALSE;
        if (!isset($form_state['storage'])) {
          $form_state['storage'] = NULL;
        }
        return $form_state;
      }
      elseif ($id == 'call_php_func_form') {
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
      return $data;
      }
    break;

    // Validate the values for an action before running the pattern
    case 'validate':
      if ($id == 'call_php_func_form') {
         _call_php_func_include_files($data);
        if (!is_callable($data['function'])) {
          return t('The given function %func is not callable', array('%func' => $data['function']));
        }
      }
    break;

    // Build a patterns actions and parameters
    case 'params':
      if ($id == 'call_php_func_form') {
        return array($data['function'], $data['type'], $data['module'], $data['name'], $data['filepath'], $data['arguments']);
      }
      elseif ($id == $data['form_id'] && isset($data['args'])) {
        $params = $data['args'];
        unset($data['args']);
        return $params;
      }
      elseif ($id == 'set_active_theme') {
        return array($data['theme_default']);
      }
    break;

    // Any last cleanup items
    case 'cleanup':
      if ($id == 'system_modules') {
        menu_rebuild();
      }
    break;
  }
}

function set_active_theme($form, $form_state, $theme_default) {
  return array(
    '#theme' => array(
      'theme_default' => $theme_default
    ),
  );
}

function set_active_theme_submit($form, &$form_state) {
  $values = $form['#theme'];
  $theme = $values['theme_default'];
  patterns_system_theme_default($theme);

}
// Modified version of system_theme_default
function patterns_system_theme_default($theme) {
  // Get current list of themes.
  $themes = list_themes();

  // Check if the specified theme is one recognized by the system.
  if (!empty($themes[$theme])) {
    // Enable the theme if it is currently disabled.
    if (empty($themes[$theme]->status)) {
    // NOTE: this assumes sites/default/css exists.
    theme_enable(array($theme));
    }
  // Set the default theme.
  variable_set('theme_default', $theme);

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
