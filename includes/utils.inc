<?php
/**
 * @file
 * Miscellaneous utility functions.
 */

define('PATTERNS_FAILED_EDIT', 'patterns_failed_edit');

/**
 * @TODO Doc.
 */
function _patterns_clear_failed_edit() {
  variable_del(PATTERNS_FAILED_EDIT);
  return TRUE;
}

/**
 * Helper function to create a context array based on the supplied object.
 *
 * Supplying a parent object will set the parent for this context.
 * @TODO Doc.
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
 * Helper function to filter values of the list of matches.
 * @TODO Doc.
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
 * Helper function to create a list of parent keys given a context item.
 * @TODO Doc.
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
 * Helper function to flatter options, but keep the title/names in.
 * @TODO Doc.
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
 * Lists the modules used by a particular pattern.
 * @TODO Doc.
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
    drupal_set_message(t('Some modules are not available, please download them before running this pattern.'), 'error');
  }
  else {
    drupal_set_message(t('All modules required by this module are available. Click !here to run this pattern.', array('!here' => l(t('here'), 'admin/patterns/enable/' . $pid))));
  }

  //Ste: was
  //return theme('table', array(t('Name'), t('Available'), t('Enabled'), t('Pattern Action')), $rows, t('Modules used for this pattern'));
  $header = array(t('Name'), t('Available'), t('Enabled'), t('Pattern Action'));
  return theme('table', array('header' => $header, 'rows' => $rows, 'caption' => t('Modules used for this pattern')));

}

/**
 * Executes default configuration for module during the module installation.
 *
 * This function should be called by other modules from within their
 * hook_enable() implementation.
 * Module should also provide modulename.config file containing PHP array
 * with the actions that need to be executed.
 *
 * @param string $module
 *   Name of the module calling the function.
 * @return
 * @TODO Doc.
 */
function patterns_execute_config($module) {

  // since this function is called from hook_enable(), we need to ensure that
  // it's executed only at module installation (first time module is enabled)
  if (drupal_get_installed_schema_version($module) == SCHEMA_INSTALLED) {
    return;
  }

  $path = drupal_get_path('module', $module) . '/' . $module . '.config';

  if (file_exists($path)) {
    require_once $path;

    if (empty($actions)) {
      return;
    }

    $pattern = new stdClass();
    $pattern->title = t('Default configuration for @module module', array('@module' => $module));
    $pattern->status = FALSE;
    $pattern->pattern['actions'] = $actions;

    return patterns_start_engine($pattern);
  }
}

/**
 * @TODO Doc.
 */
function patterns_form_helper($form, $form_state) {
  static $form_id;

  if (!$form_id && $form_state['submitted'] && !form_get_errors()) {
    $form_id = $form_state['values']['form_id'];
    $_SESSION['patterns_form_helper'] = array('form_id' => $form_id, 'form_values' => $form_state['values']);
  }

  return $form;
}



/**
 * Prints XML feed of published (public) patterns.
 * TODO: this function is outdated. It would be nice to have multiple format
 * support (YAML, XML, ...).
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
 * Installs required modules for executing the pattern.
 *
 * @param array $modules
 * @TODO Doc.
 */
function patterns_install_modules(&$modules) {
  $result = array('success' => TRUE);

  if (empty($modules)) {
    return $result;
  }

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
 * Checks if all the module dependencies are available.
 *
 * @param array $modules
 *   Array of module names.
 * @param $update_list
 *   (optional) If TRUE, add all the dependecies to pattern's module list.
 *    Defaults to FALSE.
 * @return array
 *   An empty array if all dependencies are available.
 *   Array of missing modules' names if some dependencies are not available.
 */
function patterns_check_module_dependencies(&$modules, $update_list = FALSE) {

  if (empty($modules)) {
    return array();
  }

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
 * Implements hook_token_values().
 *
 * @TODO If these get implementated directly into token.module, this should be removed.
 */
function patterns_token_values($type, $object = NULL, $options = array()) {
  if ($type == 'global') {
    $path = conf_path();
    $tokens['confpath'] = $path;
    return $tokens;
  }
}

/**
 * Array walk callback to replace tokens inside form values.
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
 * @TODO This is used instead of array_walk_recursive because of some strange
 * issues with token_get_values failing.
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
 * @TODO Find a suitable place for this, and check who is calling it.
 * @TODO Doc.
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
 * @param mixed $status One of [PATTERNS_SUCCESS, PATTERNS_WARN, PATTERNS_ERR]. Required.
 * @param array $msg A message.
 * @param mixed $result Any kind of additional data.
 *
 * @return array An associative array of the input values with keys 'status', 'msg', 'result'.
 */
function patterns_results($status = PATTERNS_SUCCESS, $msg = NULL, $result = NULL) {
  if ($msg === NULL) {
    $msg = t('Execution successful');
  }
  return array('status' => $status,
                'msg' => $msg,
                'result' => $result,
              );
}

/**
 * Checks if a (supposed) array is actually correctly formatted as a patterns_results().
 *
 * @param mixed $results The array to check.
 *
 * @return bool TRUE if correct, FALSE otherwise.
 * @see patterns_results()
 */
function _patterns_is_patterns_results($results) {
  // 'status' is the only mandatory field.
  if (!isset($results['status'])) {
    return FALSE;
  }

  return TRUE;
}

/**
 * Returns an array of allowed actions, as defined in patterns.module.
 *
 * @return array Allowed actions.
 */
function patterns_actions() {
  return array(PATTERNS_CREATE => PATTERNS_CREATE,
               PATTERNS_MODIFY => PATTERNS_MODIFY,
               PATTERNS_DELETE => PATTERNS_DELETE,
               PATTERNS_INCLUDE => PATTERNS_INCLUDE,
              );
}

/**
 * Returns a string representation of a pattern object or array.
 * If an id or the name of the pattern is passed, it tries
 * to load it, and then constructs the name of the pattern.
 *
 * @param mixed $pattern
 */
function patterns_utils_toString($pattern = NULL) {
  $pattern = _patterns_db_get_pattern($pattern);
  if (!$pattern) return FALSE;
  return $pattern->name . ' (' . $pattern->title . ')';
}


/**
 * Generate an array representing a standard info section of a pattern.
 * Default values for empty parameters are passed, e.g. the current user
 * for the author, and author's email and the url of the Drupal web site
 * for the author's web site.
 *
 * @param mixed $category
 * @param mixed $descr
 * @param mixed $v
 * @param mixed $core
 * @param mixed $user
 * @param mixed $base_root
 *
 */
// TODO: t()
function patterns_utils_generate_info_section($title = 'Untitled Pattern',
                                              $category = 'General',
                                              $descr = 'No description',
                                              $v = '1.0',
                                              $core = 'x.y',
                                              $user = NULL,
                                              $base_root = NULL ) {
  if (is_null($user)) global $user;
  if (is_null($base_root)) global $base_root;

  $info = array('info' => array());

  $info['info']['title']          = $title;
  $info['info']['author']         = $user->name;
  $info['info']['category']       = $category;
  $info['info']['description']    = $descr;
  $info['info']['core']           = $core;
  $info['info']['author']         = $user->name;
  $info['info']['author_email']   = $user->mail;
  $info['info']['author_website'] = $base_root;

  return $info;
}


/**
 * Checks a pattern identifier (numeric or alphanumeric) and if it not
 * valid displays an error message and redirect the user to the specified page.
 * If the pattern is valid, returns it.
 *
 * If no page is specified redirects to '/admin/patterns/'.
 *
 * @param mixed $pattern the pattern identifier
 * @param mixed $back the page to redirect to (default '/admin/patterns/')
 * @param array $options an associative array of extra options. E.g.
 *  $options('numeric' => TRUE) enforces that the pattern id must be numeric.
 *
 *
 */
function patterns_utils_if_invalid_go_back($pattern = NULL,
                                           $back = '/admin/patterns/',
                                           $options = array()) {

  if (isset($options['numeric'])) {
    if (!is_numeric($pattern)) {
      drupal_set_message(t('You must specify a valid id for the pattern.'));
      return FALSE;
    }
  }

  $pattern = patterns_get_pattern($pattern);

  if (!$pattern) {
    drupal_set_message(t('No pattern was found with the given id.'), 'error');
    drupal_goto($back);
  }

  return $pattern;
}

/**
 * Checks if current configuration of php enables open remote files
 * for reading.
 * 
 */
function patterns_utils_is_fopenurl_enabled() {
  return ini_get('allow_url_fopen') ? TRUE : FALSE;
}
