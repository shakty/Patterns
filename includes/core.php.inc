<?php

/**
 * 
 * Performs all the pre-operations, before doing anything serious
 * 
 * @param $pattern
 * @param $params
 */
function patterns_execute_pattern($pattern, $params = array()) {
  
   drupal_set_time_limit(0);

  if (!is_object($pattern)) {
    $pattern = patterns_get_pattern($pattern);

    if (!$pattern) {
      return FALSE;
    }
  }

  $pattern->subpatterns_run_mode = $params['run-subpatterns'];
  $pattern_details = patterns_get_pattern_details($pattern, TRUE);

  $modules = $pattern_details['modules'];
  $actions = $pattern_details['actions'];
  $actions_map = array('patterns' => $pattern_details['info'], 'map' => $pattern_details['actions_map']);
  $info = reset($pattern_details['info']);

  // If there are no actions or modules, most likely the pattern
  // was not created correctly.
  if (empty($actions) && empty($modules)) {
    drupal_set_message(t('Could not recognize pattern %title, aborting.', array('%title' => $info['title'])), 'error');
    return FALSE;
  }

  $result = patterns_install_modules($modules);
  if (!$result['success']) {
    drupal_set_message($result['error_message'], 'error');
    return FALSE;
  }

  $result = patterns_prepare_actions($actions, $actions_map);
  if (!$result['success']) {
    drupal_set_message($result['error_message'], 'error');
    return FALSE;
  }

  ///////////////////////////////
  // The fake batch
  //////////////////////////////
  for ($i = 0, $total = count($actions); $i < $total; $i++) {
    patterns_php_actions($actions[$i], $i, $actions_map);
  }

  // @TODO: return false in case of errors
  return TRUE;
}
  
function patterns_php_actions($action, $place, $actions_map) {
  
  patterns_load_components();

  // Nothing to do if there is no action
  if (empty($action)) {
    return;
  }
  $result = patterns_implement_action_php($action, $place, $actions_map);
}



/**
 * 
 * Performs all the operation necessary to build a correct form_state (op: 'validate', 'build', 'params')
 * Calls patterns_execute_action() which actually build and process the form.
 * 
 * @param unknown_type $action
 * @param unknown_type $place
 * @param unknown_type $actions_map
 */
function patterns_implement_action_php($action, $place = 0, $actions_map = NULL) {

  patterns_set_error_handler();

  $result = array('success' => TRUE);

  // Prepare actions for processing, ensure smooth pattern executions, and return form ids for execution
  $ret = patterns_invoke($action, 'form_id');

  // If prepare removed the data, dont continue with this action
  if (!$action || !$ret) {
    return $result;
  }

  if (is_string($ret)) {
    $form_ids = array($ret);
  }
  elseif ($ret) {
    $form_ids = $ret;
  }

  $action_descriptions = patterns_invoke($action, 'actions');
  $action_location = patterns_locate_action($place, $actions_map);
  $index = $action_location['key'] + 1;
  $pattern_title = $action_location['title'];
  $pattern_file = $action_location['file'];

  // Build the action
  foreach ($form_ids as $form_id) {
    $clone = $action;

    $action_description = isset($action_descriptions[$form_id]) ? $action_descriptions[$form_id] : t('System: Execute form');

    $result['action_descriptions'][$place][] = $action_description;

    // If tokens are enabled, apply tokens to the action values
    // before processing
    if (module_exists('token')) {
      _patterns_recurse_tokens($clone, $identifiers);
      //array_walk($clone, '_patterns_replace_tokens', $identifiers);
    }
    
    ///////////////////////////////////
    // VALIDATE
    //////////////////////////////////
    $error = patterns_invoke($clone, 'validate', $form_id);
    if ($message = patterns_error_get_last('validate', $index, $action_description, $pattern_title, $pattern_file)) {
      $result['error_message'] = $message;
      $result['success'] = FALSE;
      return $result;
    }

    if ($error) {
      $message = t('An error occured while validating action #%num (%action) in %title pattern', array('%num' => $index, '%action' => $action_description, '%title' => $pattern_title));
      $result['error_message'] = $message . '<br/>' . $error;
      $result['success'] = FALSE;
      return $result;
    }

    /////////////////////////////////////////////////////
    // BUILD: Get the form data for the action. This can either 
    // just be the form values, or it can be the full form_state object
    /////////////////////////////////////////////////////
    $form_obj = patterns_invoke($clone, 'build', $form_id);

    if ($message = patterns_error_get_last('build', $index, $action_description, $pattern_title, $pattern_file)) {
      $result['error_message'] = $message;
      $result['success'] = FALSE;
      return $result;
    }

    ////////////////////////////////////////////////////
    // Get any extra parameters required for the action
    ////////////////////////////////////////////////////
    $params = patterns_invoke($clone, 'params', $form_id, $action_state);
    if ($message = patterns_error_get_last('params', $index, $action_description, $pattern_title, $pattern_file)) {
      $result['error_message'] = $message;
      $result['success'] = FALSE;
      return $result;
    }

    // A single, simple value can be returned as a parameter, which is then
    // put into an array here.
    if (isset($params) && !is_array($params)) {
      $params = array($params);
    }
    
    // Dont execute the action if a string was returned, indicating the pattern component
    // most likely handled the action on its own and this is the message to display.
    if (is_string($form_obj)) {
      drupal_set_message($form_obj);
    }
    else {
      // We check for the 'storage' and 'submitted' values in the object to see
      // if it is a form_state instead of form_values. There could be a better way
      // to do this.
      if (array_key_exists('submitted', (array)$form_obj) && array_key_exists('storage', (array)$form_obj)) {
        $action_state = $form_obj;
      }
      else {
        $action_state = array(
          'storage' => NULL,
          'submitted' => FALSE,
          'build_info' => array('args' => $params, 'files' => array('modules/user/user.admin.inc')), // @TODO Change the file passed to the form
          'values' => $form_obj
        );
      }

      ////////////////////
      // Execute action
      ///////////////////
      patterns_execute_action_php($form_id, $action_state, $params);

      if ($message = patterns_error_get_last('execute', $index, $action_description, $pattern_title, $pattern_file)) {
        $result['error_message'] = $message;
        $result['success'] = FALSE;
        return $result;
      }

      if ($errors = form_get_errors()) {
        $result['error_message'] = t('Above error(s) occured while executing action #%num (%action) in %title pattern. Error location(s) are: %errors', array('%num' => $index, '%action' => $action_description, '%title' => $pattern_title, '%errors' => str_replace('][', '->', implode(', ', array_keys($errors)))));
        $result['success'] = FALSE;
        return $result;
      }

      // Let a component cleanup after each action
      patterns_invoke($clone, 'cleanup', $form_id, $action_state);
      if ($message = patterns_error_get_last('cleanup', $index,  $action_description, $pattern_title, $pattern_file)) {
          $result['error_message'] = $message;
          $result['success'] = FALSE;
          return $result;
      }
    }

    // Clear the cache in case it causes problems
    cache_clear_all();

    
  }    
  // Ste: moved outside the loop
  variable_set('menu_rebuild_needed', TRUE);
  
  patterns_restore_error_handler();

  return $result;
}



/**
 * Retrieves the form_id structure.
 * Build up the form_state
 * 
 * @param unknown_type $form_id
 * @param unknown_type $form_state
 * @param unknown_type $params
 */
function patterns_execute_action_php($form_id, &$form_state, $params) {
  // Make sure we always have a clear cache for everything.
  // Code taken from drupal_flush_all_caches().

  // Don't clear cache_form - in-progress form submissions may break.
  // Ordered so clearing the page cache will always be the last action.
  $core = array('cache', 'cache_block', 'cache_filter', 'cache_page');
  $cache_tables = array_merge(module_invoke_all('flush_caches'), $core);
  foreach ($cache_tables as $table) {
    cache_clear_all('*', $table, TRUE);
  }

  $args = array($form_id, &$form_state);

  if (is_array($params)) {
    $args = array_merge($args, $params);
  }
  
  $form = call_user_func_array('drupal_retrieve_form', $args);
  
  // Copy the #post values from the form state
  $form['#post'] = $form_state['values'];
  // Some modules depend on existence of 'post' array
  $form_state['post'] = $form_state['values'];
  
  // Ste: added
  $form_state['process_input'] = TRUE;
  
   if (!isset($form_state['build_info']['args'])) {
    $args = func_get_args();
    array_shift($args);
    array_shift($args);
    $form_state['build_info']['args'] = $args;
  }
  // Merge in default values.
  $form_state += form_state_defaults();

  // Populate $form_state['input'] with the submitted values before retrieving
  // the form, to be consistent with what drupal_build_form() does for
  // non-programmatic submissions (form builder functions may expect it to be
  // there).
  $form_state['input'] = $form_state['values'];

  $form_state['programmed'] = TRUE;
  //$form = drupal_retrieve_form($form_id, $form_state);
  // Programmed forms are always submitted.
  $form_state['submitted'] = TRUE;

  // Reset form validation.
  $form_state['must_validate'] = TRUE;
  form_clear_error();
  // Call eventual hooks that alter the form
  // Add validation token
  // Ste
  drupal_prepare_form($form_id, $form, $form_state);
  drupal_process_form($form_id, $form, $form_state);
  
}

