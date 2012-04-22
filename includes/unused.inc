<?php
/**
 * @file
 * Functions that are unused at the moment.
 */

// Forms Submission
//////////////////////////////////////////////////////


// Custom process, submit and validate.

/**
 * Custom implementation of drupal_process_form()
 *
 * Enables validation to be performed for each executed form
 * by calling our custom _patterns_validate_form() function
 * see issue: http://drupal.org/node/260934
 */
function _patterns_process_form($form_id, &$form, &$form_state) {
  //$form_state['values'] = array(); // ????

  $form = form_builder($form_id, $form, $form_state);

  // Only process the form if it is programmed or the form_id coming
  // from the POST data is set and matches the current form_id.
  //Ste: was
  //if ((!empty($form['#programmed'])) || (!empty($form['#post']) && (isset($form['#post']['form_id']) && ($form['#post']['form_id'] == $form_id)))) {
  if (TRUE) {
    _patterns_validate_form($form_id, $form, $form_state);

    // form_clean_id() maintains a cache of element IDs it has seen,
    // so it can prevent duplicates. We want to be sure we reset that
    // cache when a form is processed, so scenerios that result in
    // the form being built behind the scenes and again for the
    // browser don't increment all the element IDs needlessly.
    // Ste: was
    //form_clean_id(NULL, TRUE);
    drupal_clean_css_identifier(NULL);

    //Ste: was
    //if ((!empty($form_state['submitted'])) && !form_get_errors() && empty($form_state['rebuild'])) {
    if (TRUE) {
      $form_state['redirect'] = NULL;
      form_execute_handlers('submit', $form, $form_state);

      // We'll clear out the cached copies of the form and its stored data
      // here, as we've finished with them. The in-memory copies are still
      // here, though.
      if (variable_get('cache', CACHE_DISABLED) == CACHE_DISABLED && !empty($form_state['values']['form_build_id'])) {
        cache_clear_all('form_' . $form_state['values']['form_build_id'], 'cache_form');
        cache_clear_all('storage_' . $form_state['values']['form_build_id'], 'cache_form');
      }

      // If batches were set in the submit handlers, we process them now,
      // possibly ending execution. We make sure we do not react to the batch
      // that is already being processed (if a batch operation performs a
      // drupal_execute).
      if ($batch =& batch_get() && !isset($batch['current_set'])) {
        // The batch uses its own copies of $form and $form_state for
        // late execution of submit handers and post-batch redirection.
        $batch['form'] = $form;
        $batch['form_state'] = $form_state;
        $batch['progressive'] = !$form['#programmed'];
        batch_process();
        // Execution continues only for programmatic forms.
        // For 'regular' forms, we get redirected to the batch processing
        // page. Form redirection will be handled in _batch_finished(),
        // after the batch is processed.
      }

      // If no submit handlers have populated the $form_state['storage']
      // bundle, and the $form_state['rebuild'] flag has not been set,
      // we're finished and should redirect to a new destination page
      // if one has been set (and a fresh, unpopulated copy of the form
      // if one hasn't). If the form was called by drupal_execute(),
      // however, we'll skip this and let the calling function examine
      // the resulting $form_state bundle itself.
      if (!$form['#programmed'] && empty($form_state['rebuild']) && empty($form_state['storage'])) {
        drupal_redirect_form($form, $form_state['redirect']);
      }
    }
  }
}

/**
 * Custom implementation of drupal_validate_form()
 *
 * Removed static variable that prevented same form_id to be
 * validated more then once during a single page request
 */
function _patterns_validate_form($form_id, $form, &$form_state) {
  // If the session token was set by drupal_prepare_form(), ensure that it
  // matches the current user's session.
  if (isset($form['#token'])) {
    if (!drupal_valid_token($form_state['values']['form_token'], $form['#token'])) {
      // Setting this error will cause the form to fail validation.
      form_set_error('form_token', t('Validation error, please try again. If this error persists, please contact the site administrator.'));
    }
  }

  _form_validate($form, $form_state, $form_id);
}

// Sync

/**
 * Make some modifications to the form values based on the form
 * In particular, make sure form elements with #options and #multiple
 * set the keys of the array as the key of the value as how FAPI does it,
 * but XML of course does not.
 */
function patterns_sync_form_values($args) {
  $form_id = $args[0];

  // this functionality is not working well with
  // node forms (create/update node)
  // so skip them until this issue is fixed
  if (strpos($form_id, 'node_form') !== FALSE) {
    return;
  }

  // References inside the form_state can cause potential problems,
  // so we'll ensure no references to outside data exists
  $form_state = unserialize(serialize($args[1]));
  $form_values = $form_state['values'];
  unset($args[1]['values']);

  // Get the fully built fapi object with cloned form_state.
  // We need to do this with a separate form_state because
  // this can mess up the form values if they are setup incorrectly
  // (it'll get fixed here)
  $form = call_user_func_array('drupal_retrieve_form', $args);
  drupal_prepare_form($form_id, $form, $form_state);
  $form = form_builder($form_id, $form, $form_state);

  // Loop through all form values looking for #options
  $queue = array(&$form);

  while (!empty($queue)) {
    $check = &$queue[0];
    array_shift($queue);

    // Skip disabled items
    if ($check['#disabled']) {
      continue;
    }

    // Set default values for everything that the form is expecting values for.
    // This can save work in the components as well as avoid bugs when a module
    // expects values to be there that are not.
    // Do not set default values when it is the type of value that a form submit
    // would not set data in $_POST for. Like selects with no options or unchecked
    // checkboxes
    if ($check['#input'] && isset($check['#default_value']) &&
        !($check['#type'] == 'select' && empty($check['#options'])) &&
        !(in_array($check['#type'], array('checkbox', 'checkboxes')) && empty($check['#default_value']))) {
      $match = &$form_values;
      $found = TRUE;
      foreach ($check['#parents'] as $path) {
        if (!isset($match[$path])) {
          $found = FALSE;
          $match[$path] = array();
        }
        // If this is not an array, the data was probably not completed properly
        // in the pattern.
        if (!is_array($match)) {
          $message = t('Invalid pattern syntax at: !path. Expecting more child elements.', array('!path' => implode('->', $check['#parents'])));
          patterns_error_handler(1, $message, '', '');
          return;
        }
        $match = &$match[$path];
      }

      // Set the default value unless it is a type of value that would originally
      // not be set in $_POST during form submit, like empty selects
      if (!$found) {
        $check['#missing_in_pattern'] = TRUE;
        $match = $check['#default_value'];
      }
    }

    if (!empty($check['#options'])) {
      $scalar = FALSE;
      if ((!$check['#tree'] && !$check['#multiple']) || (isset($check['#multiple']) && !$check['#multiple']) || in_array($check['#type'], array('radio', 'radios'))) {
        $scalar = TRUE;
      }

      // Find possible corresponding data in form values
      $match = &$form_values;
      $found = TRUE;
      foreach ($check['#parents'] as $path) {
        if (isset($match[$path])) {
          $match = &$match[$path];
        }
        else {
          $found = FALSE;
          break;
        }
      }

      if ($found && isset($match)) {
        // Check if we are overwriting or not.
        //
        // First, if the pattern value is there, but empty, we obviously
        // should be overwriting. Next is to check if overwrite was specified
        // manually
        $overwrite = FALSE;
        if (!$check['#missing_in_pattern'] && (empty($match) || (is_array($match) && !empty($match['overwrite'])))) {
          $overwrite = TRUE;
          if (is_array($match) && isset($match['overwrite'])) {
            unset($match['overwrite']);
          }
        }

        $delete = FALSE;
        if (is_array($match) && !empty($match['delete'])) {
          $delete = TRUE;
          unset($match['delete']);
        }

        // If overwriting, start the values array with all the
        // possible values but empty. We will possibly fill them
        // in with default values on update actions. Select types
        // do not work when empty, non-selected, values are set
        if (!$scalar && $overwrite && $check['#type'] != 'select') {
          $values = $check['#options'];
          foreach ($values as &$v) {
            $v = '';
          }
        }
        else {
          $values = array();
        }

        if (!$overwrite && !empty($check['#default_value'])) {
          foreach ((array)$check['#default_value'] as $value) {
            $values[$value] = $value;
          }

          // Make sure we are still restricted to values available in #options
          $values = array_intersect_key($values, $check['#options']);
        }

        // Flatten out the options in case it is trying to use optgroups
        $options = patterns_options_flatten($check['#options']);

        foreach ((array)$match as $value) {
          $key = array_search($value, $options);

          // If the supplied value is an actual options value (not the label) use that
          if ((is_string($value) || is_int($value)) && array_key_exists($value, $options)) {
            $values[$value] = $delete ? 0 : $value;
          }
          // Or if the supplied value matches an options label, use that option
          elseif ($key !== FALSE) {
            $values[$key] = $delete ? 0 : $key;
          }
          // If we are trying to set an invalid value, at least select
          // the first value for select forms since it is impossible to
          // not select a good value there
          elseif ($check['#type'] == 'select') {
            reset($options);
            $values[key($options)] = key($options);
          }
        }

        // If we are not on a multiple/tree form, the value should be singular/not an array.
        // We make sure to use the last value here because the last value will absolutely
        // be the user/pattern supplied value as opposed to the default_value
        if ($scalar) {
          $match = end($values);
        }
        else {
          $match = $values;
        }
      }
    }

    foreach (element_children($check) as $element) {
      $queue[] = &$check[$element];
    }
  }

  // Make sure the new form_values will be used in the real executions
  $args[1]['values'] = $form_values;
}

/// Utils ?
///////////

/**
 * For use with token replacement
 */
function patterns_array_map($function, $array, $params) {
  $new_array = array();
  foreach ($array as $key => $value) {
    if (is_array($value)) {
      $new_array[$key] = patterns_array_map($function, $value, $params);
    }
    else {
      $params[] = $key;
      $key = call_user_func_array($function, $params);
      array_pop($params);
      $params[] = $value;
      $new_array[$key] = call_user_func_array($function, $params);
      array_pop($params);
    }
  }
  return $new_array;
}


/**
 * Take a $data and $config object and adjust $data
 * based on the supplied configuration
 *
 * TODO Look for ways to optimize and increase performance
 */
function patterns_config_data(&$data, $configs) {
  foreach ($configs as $path => $config) {
    // An alias key requires multiple paths
    // for each alias and then a #real set
    // to change all of them to the same key
    // TODO test #alias
    if ($config['#alias'] && is_array($config['#alias'])) {
      $tokens = array_slice(preg_split('/(?<!=)\/(?![a-z]*\])/', $path), 1);
      $real = end($tokens);

      // Check for the real key
      $matches = patterns_array_fetch($path, $data);

      // If the real key was not found, get the aliases
      if (empty($matches)) {
        if (!$config['#key']) {
          $config['#key'] = $real;
        }
        foreach ($config['#alias'] as $alias) {
          $tokens[count($tokens)-1] = $alias;
          $matches += patterns_array_fetch('/' . implode('/', $tokens), $data);
        }
      }
    }
    else {
      $matches = patterns_array_fetch($path, $data);
    }

    // If no matches were found and it is a required element
    // return an error
    if (empty($matches) && $config['#required']) {
      $error = TRUE;
    }

    foreach ($matches as &$match) {
      // Collect form_ids here.
      // TODO Think of a better way to migrate components form_id operations here
      if ($config['#form_id']) {
        patterns_config_form_ids($data['tag'], $config['#form_id'], $match);
      }

      // Make sure values that should be arrays are set as such
      if ($config['#array'] && !array_key_exists(0, $match['item'])) {
        if (is_null($match['item']) || (is_array($match['item'] && empty($match['item'])))) {
          $match['item'] = array();
        }
        else {
          $match['item'] = array($match['item']);
        }

        // Update the context object to reflect new changes
        $match = _patterns_array_context($match['item'], $match);
      }

      // Change the key of this match. Can get the key value via xpath as well.
      // Any duplicate keys will return an error
      if ($config['#key']) {
        // Check that a single match is found via this xpath and that the resulting value is scalar
        if ($config['#key']{0} == '/' && ($sub_matches = patterns_array_fetch($config['#key'], $match['item'], $match)) && count($sub_matches) == 1 && is_scalar($sub_matches[0]['item'])) {
          // an xpath from the matched item is used to get the key value
          $new_key = $sub_matches[0]['item'];
        }
        elseif ($config['#default key']) {
          $new_key = $config['#default key'];
        }
        elseif ($config['#key']{0} == '/') {
          // Could not find the appropriate key value from the xpath
          $error = TRUE;
        }
        else {
          // Set the key to a static value. Typically used for allowing more user readable keys
          $new_key = $config['#key'];
        }

        if ($new_key && array_key_exists($new_key, $match['parent']['item'])) {
          // The new key already exists.
          $error = TRUE;
        }
        elseif ($new_key) {
          $key = $match['key'];
          $match['parent']['item'][$new_key] = $match['item'];
          // TODO This is buggy. Skip for now.
          if (is_int($key) && FALSE) {
            array_splice($match['parent']['item'], $key, 1);
          }
          else {
            unset($match['parent']['item'][$key]);
          }

          // Update the parent context object to reflect new changes
          $match['parent'] = _patterns_array_context($match['parent']['item'], $match['parent']);

          // Update to the new current match
          for ($i=0, $total = count($match['parent']); $i < $total; $i++) {
            if ($match['parent'][$i]['key'] == $new_key) {
              $match = &$match['parent'][$i];
              break;
            }
          }
        }
      }

      // Create empty key elements if they don't exist but should (and are not required)
      // This is useful when using 'move' to ensure that the destination exists
      if ($config['#create'] && is_array($match['item']) && !array_key_exists($config['#create'], $match['item'])) {
        $match['item'][$config['#create']] = $config['#create value'];
      }

      // Move(or copy) this match to somewhere else in the object. If more than one destination
      // match is found, this value is copied to each one
      // TODO Make move automatically create the destination if it doesn't exist
      // TODO Support move in (put inside array) and move to (replace) actions
      if (!empty($match['parent']) && ($config['#move']{0} == '/' && ($dest_path = $config['#move']) && ($op = 'move')) ||
      ($config['#copy']{0} == '/' && ($dest_path = $config['#copy']) && ($op = 'copy'))) {
        $dests = patterns_array_fetch($dest_path, $match['item'], $match);
        $parent = &$match['parent'];
        $obj = $match['item'];

        foreach ($dests as $dest) {
          // Can't move to items that are not an array
          if (!is_array($dest['item'])) {
            continue;
          }
          if (!is_int($match['key'])) {
            $dest['item'][$match['key']] = $obj;
          }
          else {
            $dest['item'][] = $obj;
          }

          // Update the dest object to reflect the change
          $dest = _patterns_array_context($dest['item'], $dest);
        }

        if ($op == 'move') {
          if (!is_int($match['key'])) {
            unset($parent['item'][$match['key']]);
          }
          else {
            array_splice($parent['item'], $match['key'], 1);
          }

          // Update the parent object to reflect the change (removal)
          $parent = _patterns_array_context($parent['item'], $parent);
          unset($match);
        }
      }
    }
  }
}

// Stuff ?
///////////

function patterns_exit($destination = NULL) {
  $batch =& batch_get();
  if (variable_get('patterns_form_helper', FALSE) && $_SESSION['patterns_form_helper'] && !$destination && empty($batch)) {
    if (module_exists('devel')) {
      //dpm($_SESSION['patterns_form_helper']);
      kprint_r($_SESSION['patterns_form_helper']);
    }
    else {
      print theme('patterns_form_helper', $_SESSION['patterns_form_helper']['form_id'], $_SESSION['patterns_form_helper']['form_values']);
    }

  }
}


/**
 * Function callback
 */
function _patterns_modify_value(&$form) {
  foreach ($form as $key => $value) {
    if (is_array($value) && isset($value['#type']) && $value['#type'] == 'value') {
      $form[$key]['#default_value'] = $value['#value'];
      unset($form[$key]['#value']);
    }
    elseif (is_array($value)) {
      _patterns_modify_value($form[$key]);
    }
  }
}


/**
 * @file
 * Functions related to parsing pattern files.
 */

/**
 * Finds parts of an array based on a semi-compatible Xpath syntax.
 *
 * Returns an array of constructs that includes the references 'item' and
 * 'parent' from the matching values in the $data object along with extra keys
 * 'key' for the key of the current match and 'trace' for a full list of keys
 * until the root of the $data object.
 *
 * Loosely based off of Cake function Set::extract.
 *
 * @Note: Ensure this always only returns matches from a single level in the
 * array. Changes made to matches in different levels can possibly mess up the
 * above-level matches. To this end, you cannot change the keys of a current
 * match because the other matches cannot change their keys as well to sync with
 * the new parent array.
 * @TODO: Think of a solution for the above note (BROKEN RIGHT NOW).
 */
function patterns_array_fetch($path, &$data, $context = NULL) {
  if (!$context && (empty($data) || !is_array($data))) {
    return array();
  }
  if ($path === '/') {
    return $data;
  }

  // Construct our contexts object that allows us to traverse the array
  if (!$context) {
    $context = _patterns_array_context($data);
  }

  // Make our context actually a list of contexts
  $context = array($context);

  // Create a list of tokens based on the supplied path
  $tokens = array_slice(preg_split('/(?<!=)\/(?![a-z]*\])/', $path), 1);

  while (!empty($tokens)) {
    $token = array_shift($tokens);

    // TODO Implement better conditionals for each token
    // Currently only supports element=value conditions
    $conditions = array();
    if (preg_match('/(=)(.*)/', $token, $m)) {
      $conditions[$m[1]] = $m[2];
      $token = substr($token, 0, strpos($token, $m[1]));
    }

    $matches = array();
    foreach ($context as &$piece) {
      if ($token === '..') {
        $matches[] = &$piece['parent'];
        continue;
      }
      $match = FALSE;
      if (is_array($piece['item']) && ($token == '*' || array_key_exists($token, $piece['item']))) {
        $i = 0;
        while (isset($piece[$i])) {
          if ($piece[$i]['key'] === $token) {
            $matches[] = &$piece[$i];
            break;
          }
          elseif ($token === '*') {
            $matches[] = &$piece[$i];
          }
          $i++;
        }
      }
      elseif ($token === '.') {
        $matches[] = &$piece;
      }
    }

    // Filter matches from the matches list based on our conditions
    foreach ($conditions as $operator => $value) {
      _patterns_array_filter($matches, $operator, $value);
    }

    // Update the context area to the next set of matches to dig into
    $context = $matches;
  }

  // Return the list of matches containing references to their respective data objects
  return $matches;
}

// Moved here recently
//////////////////////


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


// Function which were originally commented
////////////////////////////////////////////


//function patterns_debug_batch($var = NULL) {
//  $var = print_r($var, TRUE);
//  $var = str_replace(" ", '&nbsp;', $var);
//  $var = str_replace("\n", "<br />", $var);
//  print drupal_to_js(array('status' => 0, 'data' => $var));
//  exit;
//}


// function patterns_revert($pid) {
//   if ($name = db_result(db_query('SELECT name FROM {patterns} WHERE pid = "%d"', $pid))) {
//     $path = file_create_path(variable_get('patterns_save_file', 'patterns') .'/enabled/'. $name .'.xml');
//     $new = db_result(db_query('SELECT file FROM {patterns} WHERE pid = "%d"'));
//   }
//   else {
//     drupal_set_message(t('The pattern you specified does not exist.'), 'error');
//     drupal_goto('admin/patterns');
//   }
//
//   if (file_exists($path)) {
//     if (file_move($path, $new, FILE_EXISTS_REPLACE)) {
//       drupal_set_message(t('This pattern was reverted to the state it was at when it was enabled.'));
//       drupal_goto();
//     }
//   }
//   else {
//     drupal_set_message(t('The patterns enabled-state was not saved properly, therefore it cannot be reverted.'), 'error');
//   }
//
//   drupal_goto('admin/patterns');
// }

/**
 * Menu callback to display patterns details page
 */
// function patterns_info($pid = null) {
//   if (!is_numeric($pid)) {
//     drupal_set_message(t('You must specify a pattern.'));
//     return;
//   }
//
//   $pattern = patterns_get_pattern($pid);
//
//   $output = '';
//   return $output;
// }



// function patterns_disable_pattern($pid) {
//     $form['pid'] = array(
//     '#type' => 'value',
//     '#value' => $pid
//   );
//
//   $pattern = patterns_get_pattern($pid);
//
//   return confirm_form($form, t('Proceed with disabling pattern %pattern?', array('%pattern' => $pattern->title)), 'admin/patterns', '');
// }

//function patterns_process_modules($modules, $op = 'enable') {
//  // Enable at the beginning of the pattern. Disable at the end.
//  for($i=0;$module=$modules[$i];$i++) {
//    if (($op == 'enable' && $module['delete']) || ($op == 'disable' && !$module['delete'])) {
//      unset($modules[$i]);
//    }
//  }
//
//  patterns_invoke($empty, 'tag modules');
//  $error = patterns_invoke($modules, 'prevalidate');
//
//  // Error validating modules
//  if ($error) {
//    return $error;
//  }
//
//  patterns_invoke($modules, 'prepare');
//}

// function patterns_disable_pattern_submit($form_id, $form_values) {
//   $pid = $form_values['pid'];
//   $pattern = patterns_get_pattern($pid);
//
//   if (patterns_execute_pattern($pattern, true, true)) {
//     return 'admin/patterns';
//   }
// }
