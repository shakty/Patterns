<?php


define('PATTERNS_FAILED_EDIT', 'patterns_failed_edit');

function _patterns_clear_failed_edit(){
	variable_del(PATTERNS_FAILED_EDIT);
	return true;
}

/**
 * Helper function to create a context array based on the supplied object
 * Supplying a parent object will set the parent for this context
 */
function _patterns_array_context(&$obj, &$current = NULL) {
  // If a current context is set, use it's parent and key values
  if (!($trace = $current['trace'])) {
    $trace = array();
  }
  if (!($key = $current['key'])) {
    $key = NULL;
  }
  if (!($parent = &$current['parent'])) {
    $parent = NULL;
  }
  $context = array('trace' => $trace, 'key' => $key, 'item' => &$obj, 'parent' => &$parent);
  $refs = array(&$context);

  while (!empty($refs)) {
    $ref = &$refs[0];
    $parent = &$ref['item'];
    array_splice($refs, 0, 1);

    if (is_array($parent) && !empty($parent)) {
      $i = 0;
      foreach ($parent as $index => &$child) {
        // TODO possible optimizations can be done here (with the parent trace)
        $ref[$i] = array('trace' => _patterns_array_trace($ref), 'key' => $index, 'item' => &$child, 'parent' => &$ref);
        array_unshift($refs, '');
        $refs[0] = &$ref[$i++];
      }
    }
  }

  return $context;
}

/**
 * Helper function to filter values of the list of matches
 */
function _patterns_array_filter(&$matches, $operator, $value = NULL) {
  for ($i = count($matches) - 1; $i >= 0; $i--) {
    $match = &$matches[$i];

    switch ($operator) {
      case '=':
        if ($match['item'] != $value) {
          array_splice($matches, $i, 1);
        }
        break;
    }
  }
}

/**
 * Helper function to create a list of parent keys given a context item
 */
function _patterns_array_trace($obj) {
  // Loop back up through the parents to fill in the trace value.
  $up = &$obj;
  $trace = array();
  while (isset($up['parent'])) {
    array_unshift($trace, $up['key']);
    $up = &$up['parent'];
  }

  return $trace;
}


/**
 * Helper function to flatter options, but keep the title/names in
 */
function patterns_options_flatten($array, $reset = TRUE) {
  static $return;

  if ($reset) {
    $return = array();
  }

  foreach ($array as $key => $value) {
    if (is_object($value)) {
      patterns_options_flatten($value->option, FALSE);
    }
    elseif (is_array($value)) {
      patterns_options_flatten($value, FALSE);
    }
    else {
      $return[$key] = $value;
    }
  }

  return $return;
}

/**
 * List the modules used by a particular pattern
 */
function patterns_modules_page($pid) {
  $pattern = patterns_get_pattern($pid);

  drupal_set_title($pattern->title . ' ' . t('(Pattern Modules)'));

  $modules = isset($pattern->pattern['modules']) ? $pattern->pattern['modules'] : array();
  $modules_info = system_rebuild_module_data();
  $modules_list = module_list();

  // Get module name, whether its to be disabled or enabled,
  // whether the module is available or not, and whether it is
  // currently enabled or not
  foreach ($modules as $module) {
    $row = array();
    $delete = is_array($module) ? $module['delete'] : FALSE;
    $module = is_array($module) ? $module['value'] : $module;
    $available = array_key_exists($module, $modules_info);
    $enabled = array_key_exists($module, $modules_list);
    $row[] = $module;
    $row[] = $available ? t('Yes') : '<span class="alert">' . t('No') . '</span>';
    $row[] = $enabled ? t('Yes') : '<span class="alert">' . t('No') . '</span>';
    $row[] = $delete ? t('Delete') : t('Enable');
    
    // Ste: was
    //$rows[] = $row;
    $rows['data'] = $row;
        
    if (!$available) {
      $not_available = TRUE;
    }
  }

  if ($not_available) {
    drupal_set_message(t('Some modules are not availalble, please download them before running this pattern.'), 'error');
  }
  else {
    drupal_set_message(t('All modules required by this module are available. Click !here to run this pattern.', array('!here' => l(t('here'), 'admin/patterns/enable/' . $pid))));
  }

  //Ste: was
  //return theme('table', array(t('Name'), t('Available'), t('Enabled'), t('Pattern Action')), $rows, t('Modules used for this pattern'));
  $header = array(t('Name'), t('Available'), t('Enabled'), t('Pattern Action'));
  return theme('table', array('header' => $header, 'rows' => $rows, 'caption' => 'Modules used for this pattern')); //
  
}

/**
 * Execute default configuration for module during the module installation
 *
 * This function should be called by other modules from
 * within their hook_enable() implementation.
 * Module should also provide modulename.config file containing PHP array
 * with the actions that need to be executed.
 *
 * @param $module
 *   name of the module calling the function
 */
function patterns_execute_config($module) {

  // since this function is called from hook_enable(), we need to ensure that
  // it's executed only at module installation (first time module is enabled)
  if (drupal_get_installed_schema_version($module) == SCHEMA_INSTALLED) return;

  $path = drupal_get_path('module', $module) . '/' . $module . '.config';

  if (file_exists($path)) {
    include_once($path);

    if (empty($actions)) return;

    $pattern = new stdClass();
    $pattern->title = t('Default configuration for @module module', array('@module' => $module));
    $pattern->status = FALSE;
    $pattern->pattern['actions'] = $actions;
    
    patterns_execute_pattern($pattern);
  }
}

function patterns_form_helper($form, $form_state) {
  static $form_id;

  if (!$form_id && $form_state['submitted'] && !form_get_errors()) {
    $form_id = $form_state['values']['form_id'];
    $_SESSION['patterns_form_helper'] = array('form_id' => $form_id, 'form_values' => $form_state['values']);
  }

  return $form;
}



/**
 * Prints XML Feed of published (public) patterns
 *
 */
function patterns_feed() {
  global $base_url;

  if (!variable_get('patterns_allow_publish', FALSE)) {
    return drupal_not_found();
  }

  $patterns = variable_get('patterns_allow_publish', FALSE) ? patterns_get_patterns() : array();

  foreach ($patterns as $pattern) {
    if (!$pattern->public) {
      continue;
    }
    preg_match('/[^\.]*$/', $pattern->file, $matches);
    $extension = $matches[0];
    $item = "    <pattern>\n";
    $item .= "      <pid>" . $pattern->pid . "</pid>\n";
    $item .= "      <name>" . $pattern->name . "</name>\n";
    $item .= "      <title>" . $pattern->title . "</title>\n";
    $item .= "      <description>" . $pattern->description . "</description>\n";
    $item .= "      <category>" . $pattern->pattern['info']['category'] . "</category>\n";
    $item .= "      <file_type>" . $extension . "</file_type>\n";
    $item .= "    </pattern>\n";

    $items .= $item;
  }

  $header = "  <info>\n";
  $header .= "    <url>" . $base_url . "</url>\n";
//  $header .= "    <description>". $description ."</description>\n";
  $header .= "  </info>\n";

  $feed = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  $feed .= "<patterns_feed>\n";
  $feed .= $header;
  $feed .= "  <patterns>\n";
  $feed .= $items;
  $feed .= "  </patterns>\n";
  $feed .= "</patterns_feed>\n";

  drupal_add_http_header('Content-Type', 'text/xml; charset=utf-8');
  print $feed;
  exit;
}


/**
 * Install required modules for executing the pattern.
 * 
 * @param array $modules
 */
function patterns_install_modules(&$modules) {
  $result = array('success' => TRUE);

  if (empty($modules)) return $result;

  $missing = patterns_check_module_dependencies($modules, TRUE);
  if (!empty($missing)) {
    $result['success'] = FALSE;
    $result['error_message'] = t('Following required modules are missing: %modules', array('%modules' => implode(', ', $missing)));
    $result['missing_modules'] = $missing;
    return $result;
  }

  require_once DRUPAL_ROOT . '/' . 'includes/install.inc';
  module_enable($modules, TRUE);
  system_rebuild_module_data();

  $result['installed_modules'] = $modules;

  return $result;
}

/**
 * Check if all the module dependencies are available
 *
 * @param $modules
 *   array of module names
 * @param $update_list
 *   if TRUE, add all the dependecies to pattern's module list
 * @return
 *   empty array if all dependencies are available
 *   array of missing module's names if some dependencies are not available
 *
 */
function patterns_check_module_dependencies(&$modules, $update_list = FALSE) {

  if (empty($modules)) return array();

  // Ste: was
  //$modules_info = module_rebuild_cache();
  $modules_info = system_rebuild_module_data();
  
  $result = array();
  $dependencies = array();

  foreach ($modules as $module) {
    $module = is_array($module) ? $module['value'] : $module;
    if (array_key_exists($module, $modules_info)) {
      // check also for module's dependencies
      foreach ($modules_info[$module]->info['dependencies'] as $dependency) {
        if (array_key_exists($dependency, $modules_info)) {
          $dependencies[] = $dependency;
        }
        else {
          $result[] = $dependency;
        }
      }
    }
    else {
      $result[] = $module;
    }
  }

  if ($update_list && empty($result) && !empty($dependencies)) {
    $modules = array_unique(array_merge($modules, $dependencies));
  }

  return $result;
}

// Tokens

/**
 * Implementation of hook_token_values()
 *
 * @If these get implementated directly into token.module, this should be removed
 */
function patterns_token_values($type, $object = NULL, $options = array()) {
  if ($type == 'global') {
    $path = conf_path();
    $tokens['confpath'] = $path;
    return $tokens;
  }
}

/**
 * Array walk callback to replace tokens inside form values
 */
function _patterns_replace_tokens(&$a, &$b, $identifiers = array()) {
  static $count = 0;

  // Replace IDs with identifiers from the current executing pattern
  if (preg_match('/@([a-zA-Z0-9_]+)@/', $a, $match)) {
    $a = str_replace($match[0], $identifiers[$match[1]], $a);
  }
  if (preg_match('/__([a-zA-Z0-9_]+)__/', $b, $match)) {
    $b = str_replace($match[0], $identifiers[$match[1]], $a);
  }

  // Replace tokens
  // Ste: was
  //$a = token_replace($a, 'global', NULL, '@', '@');
  //$b = token_replace($b, 'global', NULL, '__', '__');\
  
  //$a = token_replace($a, 'global', NULL, '@', '@');
  //$b = token_replace($b, 'global', NULL, '__', '__');
  
  
}

/**
 * Recurse an array and replace with tokens
 * @ This is used instead of array_walk_recursive because
 *   of some strange issues with token_get_values failing.
 */
function _patterns_recurse_tokens(&$object, $identifiers) {
  foreach ($object as $key => $value) {
    if (is_array($value)) {
      _patterns_recurse_tokens($object[$key], $identifiers);
    }
    elseif (is_scalar($value)) {
      $old_key = $key;
      _patterns_replace_tokens($object[$key], $key, $identifiers);

      // The key was changed, change it
      if ($old_key != $key) {
        $keys = array_keys($object);
        $keys[array_search($old_key, $keys)] = $key;
        $object = array_combine($keys, array_values($object));
      }
    }
  }
}


/**
 * 
 * @TODO: find a suitable place for this, and check who is calling it
 * 
 * @param unknown_type $b
 */
function patterns_executing($b = NULL) {
  static $executing = FALSE;

  if (is_bool($b)) {
    $executing = $b;
  }

  return $executing;
}

/**
 * Helper function to get PHPDoc @version tag from a file
 */
function _get_file_phpdoc_version($path) {
  $version = 'unknown';
  $needle  = '@version ';

  if (file_exists($path)) {
    $fp = @fopen($path, 'r');
    if ($fp) {
      while (!feof($fp)) {
        $occurence = stristr(fgets($fp), $needle);
        if ($occurence) { // FALSE if stristr found nothing
          return rtrim(substr($occurence, strlen($needle)));
        }
      }
    }
  }
  return $version;
}

/**
 * Helper function to create the associative array of returned values from
 * an invoked function.
 * 
 * @param mixed $status [PATTERNS_SUCCESS,PATTERNS_WARN,PATTERNS_ERR]
 * @param array $msg the msg and its type 
 * @param mixed $result Extra data to be returned
 */
function patterns_results ($status = PATTERNS_SUCCESS, $msg = 'Execution successful', $result = NULL) {
  // TODO: t($msg)
  return array( 'status' => $status,
                'msg' => $msg,
                'result' => $result,
              );
}
/**
 * Check if a (supposedly) array is actually correctly formatted as a patterns_results
 * 
 * @param mixed $results the array to check;
 * 
 * @return TRUE if correct, FALSE otherwise
 */
function _patterns_is_patterns_results ($results) {
  
  // It is the only mandatory field
  if (!isset($results['status'])){  
    return FALSE;
  }
 
  return TRUE;
}

/**
 * Returns an array of allowed actions, as defined in patterns.module
 * 
 * @return array Allowed actions
 */
function patterns_actions() {
  return array( PATTERNS_CREATE => PATTERNS_CREATE, 
                PATTERNS_MODIFY => PATTERNS_MODIFY, 
                PATTERNS_DELETE => PATTERNS_DELETE,
               );
}
