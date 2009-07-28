<?php

function system_patterns($op, $id = null, &$data = null) {
  static $modules_info;

  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('modules', 'form', 'variables', 'variable', 'theme');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'system_modules' => t('System: Enable or disable module'),
        'variables' => t('System: Set or delete variables'),
        'variable' => t('System: Set or delete variable'),
        'system_themes_form' => t('System: Set active theme'),
        'form' => t('System: Execute form')
      );
    break;

    // Return a summary of an action
    case 'summary':
      switch($id) {
        case 'system_modules':
          $enable = $disable = array();
          for($i=0;$item=$data[$i];$i++) {
             if ($item[$i]['delete']) {
               $disable[] = $data[$i]['value'];
             }
             else {
               $enable[] = $data[$i]['value'];
             }
          }

          $enable = implode(', ', $enable);
          $disable = implode(', ', $disable);

          return t('Enabling modules: %enable', array('%enable' => $enable)) .'<br />'. t('Disabling modules: %disable', array('%disable' => $disable));
        break;
        case 'variables':
          for($i=0;$item=$data[$i];$i++) {
             $variables[] = $item['name'];
          }

          return t('Setting variables: %vars', array('%vars' => $variables));
        break;
        case 'variable':
          return t('Setting variable %var', array('%var' => $data['name']));
        break;
        case 'system_themes_form':
          return t('Setting theme to %name', array('%name' => $data['value']));
        break;
        case 'form':
          return t('Executing form: %name', array('%name' => $data['form_id']));
        break;
      }
    break;

    // Prepare data for processing
    case 'prepare':
      switch($id) {
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

        	for($i=0;$i<count($data);$i++) {
        		$item = &$data[$i];

            if (!isset($item['value']) && !$item['delete']) {
            	$value = $item;
              unset($value['name']);
              $item = array('name' => $item['name'], 'value' => $value);
            }
        	}
        break;
        case 'modules':
          // Make a <modules>modulename</modules> style tag work
          if (is_string($data) || ($data['value'] && !$data[0])) {
            $data = array($data);
          }

          // Ensure proper data format for cases when "delete" (disable module) option is used
          // or when <modules> tag contains only single <module> tag
          if (!empty($data['module']) && is_array($data['module']) && count($data) == 1) {
            if (isset($data['module'][0])) {
              $data = $data['module'];
            }
            elseif (isset($data['module']['value'])) {
              $data[0] = $data['module'];
            }
            unset($data['module']);
          }
          elseif (!empty($data['module']) && is_string($data['module']) && count($data) == 1) {
            $data[0]['value'] = $data['module'];
            unset($data['module']);
          }

          for($i=0;$item=$data[$i];$i++) {
            // Ensure that modules with tags like <module>modulename</module>
            // are represented as an array instead of a string
            if (is_string($item)) {
              $data[$i] = array('value' => $item);
            }
            // proccess alias for delete
            if (isset($item['disable']) && !isset($item['delete'])) {
              $data[$i]['delete'] = $data[$i]['disable'];
            }
            unset($data[$i]['disable']);
          }
        break;
        case 'theme':
        	if ($data['value']) {
        		$data['theme_default'] = $data['value'];
            $data['status'][$data['value']] = $data['value'];
        	}
        break;
        case 'form':
          // attempt to load required include file
          $menu = menu_router_build(TRUE);
          foreach ($menu as $item) {
            if ($item['page arguments'][0] == $data['form_id']) {
              $data['include'] = $item['include file'];
              break;
            }
          }
          if (!empty($data['include']) && is_file($data['include'])) {
            require_once $data['include'];
          }
          elseif (is_array($data['include'])) {
            $data['include'] = drupal_get_path('module', $data['include']['module']) .'/'. $data['include']['file'];
          }

          if (!function_exists($data['form_id'])) {
            if (!empty($data['module']) && is_string($data['module'])) {
              $modules = module_list();
              if (in_array($data['module'], $modules)) {
                // try most common include file names
                module_load_include('inc', $data['module']);
                module_load_include('inc', $data['module'], $data['module'] .'.admin');
                module_load_include('inc', $data['module'], $data['module'] .'.page');
              }
            }
          }
        break;
      }
    break;

    // Pre validate actions
    case 'pre-validate':
      if ($id == 'modules') {
        if (!is_array($modules_info)) {
          $modules_info = module_rebuild_cache();
        }

        $modules = module_list();
        for($i=0;$item=$data[$i];$i++) {
          $module = $item['value'];

          // Ensure a module can be disabled safely
          if ($item['delete'] && array_key_exists($module, $modules)) {
            foreach((array)$modules_info[$module]->info['dependents'] as $dependent) {
              if (array_key_exists($dependent, $modules)) {
                $remove[] = $i;
                drupal_set_message(t('Warning: Could not disable %module because other modules depend on it.', array('%module' => $module)), "warning");
                break;
              }
            }
          }
          // Ensure a module and all of its dependencies exist
          else if (!array_key_exists($module, $modules)) {
            if (!array_key_exists($module, $modules_info)) {
              $required[] = $module;
            }

            foreach((array)$modules_info[$module]->info['dependencies'] as $dependency) {
              if (!array_key_exists($dependency, $modules) && !array_key_exists($dependency, $modules_info)) {
                $required[] = $dependency;
              }
            }

            if (!empty($required)) {
              return t('%module can not be installed because the module or its dependencies are missing. Please download them and try again.', array('%module' => $module)) .
                     t('!title%dependencies', array('!title' => '<br /><b>'. t('Missing module(s): ') .'</b>', '%dependencies' => implode(', ', $required)));
            }
          }
        }
        if (!empty($remove)) {
          foreach ($data as $key => $item) {
            if (!in_array($key, $remove)) $result[] = $item;
          }
          $data = $result;
        }
      }
      else if ($id == 'theme') {
      	$themes = system_theme_data();
        if (!array_key_exists($data['theme_default'], $themes)) {
        	return t('%theme is not a valid theme.', array('%theme' => $data['theme_default']));
        }
      }
      else if ($id == 'form') {
      	if (!array_key_exists('form_id', $data)) {
      		return t('"form_id" is missing.');
      	}
      	if (!function_exists($data['form_id'])) {
          return t("Couldn't load the form %form. Check if all required modules are enabled and try to define 'include' or 'module' for this action.", array('%form' => $data['form_id']));
      	}
      }
    break;

    // Return the form_id('s) for each action
    case 'form_id':
      if ($id == 'modules') {
        return 'system_modules';
      }
      else if ($id == 'variables' || $id == 'variable') {
        return 'variables';
      }
      else if ($id == 'theme') {
      	return 'system_themes_form';
      }
      else if ($id == 'form') {
        if (!empty($data['include']) && is_file($data['include'])) {
          require_once $data['include'];
        }
        elseif (!empty($data['module']) && is_string($data['module'])) {
          $modules = module_list();
          if (in_array($data['module'], $modules)) {
            // try most common include file names
            module_load_include('inc', $data['module']);
            module_load_include('inc', $data['module'], $data['module'] .'.admin');
            module_load_include('inc', $data['module'], $data['module'] .'.page');
          }
        }
      	return $data['form_id'];
      }
    break;

    // Prepare for valid processing of this type of component
    case 'build':
      if ($id == 'system_modules') {
        module_load_include('inc', 'system', 'system.admin');
        $modules = module_list();

        $data['status'] = $modules;

        if (!is_array($modules_info)) {
          $modules_info = module_rebuild_cache();
        }

        for($i=0;$item=$data[$i];$i++) {
          $module = $item['value'];

          // Set all the dependencies for the module
          if (!$item['delete']) {
            foreach((array)$modules_info[$module]->info['dependencies'] as $dependency) {
              if (!array_key_exists($dependency, $modules)) {
                $data['status'][$dependency] = $dependency;
              }
            }

            $data['status'] += array($module => $module);
          }
          else {
            unset($data['status'][$module]);
          }

          unset($data[$i]);
        }

        return $data;
      }
      else if ($id == 'variables') {
        $names = array();
        for($i=0;$variable=$data[$i];$i++) {
          if ($variable['delete']) {
            variable_del($variable['name']);
          }
          else {
            $var = variable_get($variable['name'], NULL);
            if (is_array($var)) {
              // make sure we don't loose parts of the array that were not defined by pattern's action
              $var = array_merge($var, $variable['value']);
              variable_set($variable['name'], $var);
            }
            else {
              variable_set($variable['name'], $variable['value']);
            }
          }
          $names[] = $variable['name'];
        }

        return t('Variables %vars updated.', array('%vars' => implode(', ', $names)));
      }
      else if ($id == 'system_themes_form') {
        $data['op'] = t('Save configuration');
        module_load_include('inc', 'system', 'system.admin');
      	return $data;
      }
      else if ($id == $data['form_id']) {
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
    break;

    // Validate the values for an action before running the pattern
    case 'validate':
    break;

    // Build a patterns actions and parameters
    case 'params':
      if ($id == $data['form_id'] && isset($data['args'])) {
        $params = $data['args'];
        unset($data['args']);
        return $params;
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
