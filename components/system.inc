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
        'system_modules' => t('Enable/Disable Module'),
        'variables' => t('Configure Variables'),
        'variable' => t('Set a variable'),
        'system_themes' => t('Set active theme'),
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
        case 'system_themes':
          return t('Setting theme to %name', array('%name' => $data['value']));
        break;
      }
    break;
    
    // Prepare data for processing
    case 'prepare':
      // Turn <variable>value</variable> style tag function the same as <variables>
      if ($id == 'variable') {
        $id = 'variables';
        $data = array($data);
      }
      
      // Make a <variables>modulename</variables> style tag work
      if ($id == 'variables' && !$data[0]) {
        if ($data['variable']) {
          $data = $data['variable'];
          unset($data['variable']);
        }
        else {
          $temp = $data;
          $data[0] = $temp;
          unset($data['id'], $data['value'], $data['delete']);          
        }
      }
      else if ($id == 'modules' && is_string($data)) {
        $data = array($data);
      }
      
      if ($id == 'variables') {
      	for($i=0;$i<count($data);$i++) {
      		$item = &$data[$i];
          
          if (!$item['value']) {
          	$value = $item;
            unset($value['name']);
            $item = array('name' => $item['name'], 'value' => $value);
          }
      	}
      }

      if ($id == 'modules') {
        // Make a <modules>modulename</modules> style tag work
        if (is_string($data) || ($data['value'] && !$data[0])) {
          $data = array($data);
        }

        // Ensure that modules with a tags like <module>modulename</module
        // are represented as an array instead of a string
        for($i=0;$item=$data[$i];$i++) {
          if (is_string($item)) {
            $data[$i] = array('value' => $item);
          }
        }
      }
      
      if ($id == 'theme') {
      	if ($data['value']) {
      		$data['theme_default'] = $data['value'];
          $data['status'][$data['value']] = $data['value'];
      	}
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
                array_splice($data, $i, 1);
                drupal_set_message(t('Warning: Could not disable %module because modules depend on it.', array('%module' => $module)));
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
                     t('!title%dependencies', array('!title' => '<br /><b>'. t('Missing: ') .'</b>', '%dependencies' => implode(', ', $required)));
            }
          }
        }
      }
      else if ($id == 'theme') {
      	$themes = system_theme_data();
        if (!array_key_exists($data['theme_default'], $themes)) {
        	return t('%theme is not a valid theme.', array('%theme' => $data['theme_default']));
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
      	return 'system_themes';
      }
    break;
    
    // Prepare for valid processing of this type of component
    case 'build':

      if ($id == 'system_modules') {
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
            variable_set($variable['name'], $variable['value']);
          }
          
          $names[] = $variable['name'];
        }
        
        return t('Variables %vars updated.', array('%vars' => implode(', ', $names)));
      }
      else if ($id == 'system_themes') {
        $data['op'] = t('Save configuration');
      	return $data;
      }
    break;
    
    // Validate the values for an action before running the pattern
    case 'validate':
      
    break;
    
    // Build a patterns actions and parameters
    case 'params':
    break;
    
    // Reverse actions when disabling a pattern
    case 'reverse':
      if ($id == 'modules') {
        for($i=0;$item=$data[$i];$i++) {
          if ($data[$i]['delete']) {
            unset($data[$i]['delete']);
          } 
          else {
            $data[$i]['delete'] = true;
          }
        }
      }
      else if ($id == 'variables') {
        for($i=0;$variable=$data[$i];$i++) {
          if ($variable['delete']) {
            unset($data[$i]['delete']);
          }
          else {
            $data[$i]['delete'] = true;
          }
        }
      }
      else {
        return false;
      }
      
      return $data;
    break;
    
  }
}