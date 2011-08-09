<?php 

/**
 * Starts preliminary operations for pattern execution.
 * 
 * E.g.: loading additional modules, and creating the array of patterns actions.
 * 
 * If there are no errors, it creates the batch array of operations. Each of them
 * is a call to patterns_batch_actions with different parameters.
 * 
 * @param stdClass $pattern Pattern object as loaded by patterns_get_pattern()
 * @param array $params Parameters for executing the pattern. Array format as follows:
 * pid => Pid of the pattern as it is in the database  
 * run-subpatterns => ['first-update', always','update','first','never']
 * 
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
  
  // Install modules, if necessary
  $result = patterns_install_modules($modules);
  if (!$result['success']) {
    drupal_set_message($result['error_message'], 'error');
    return FALSE;
  }
  
  ////////////////////////////////
  // 'prepare' and 'pre-validate'
  ///////////////////////////////
  
  // $actions passed as reference
  $result = patterns_prepare_actions($actions, $actions_map);
  if (!$result['success']) {
    drupal_set_message($result['error_message'], 'error');
    return FALSE;
  }

  $batch = array(
    'title' => t('Processing pattern %pattern', array('%pattern' => $info['title'])),
//    'init_message' => t('Running action @current out of @total', array('@current' => 1, '@total' => count($actions))),
    'progress_message' => t('Running action @current out of @total'),
    'operations' => array(),
    'file' => drupal_get_path('module', 'patterns') . '/includes/core.batch.inc',
    'finished' => 'patterns_batch_finish'
  );

  for ($i = 0, $total = count($actions); $i < $total; $i++) {
    $batch['operations'][] = array('patterns_batch_actions', array($actions[$i], $i, $actions_map));
  }

  $_SESSION['patterns_batch_info'] = $pattern_details['info'];

  batch_set($batch);
  return TRUE;
}


/**
 * 
 * Execute a batch action.
 * 
 * @TODO improve documentation
 * 
 * 
 * @param array $action
 * @param mixed $place index of the current operation within the batch_set
 * @param array $actions_map [$pid pattern id, $index ??]
 * @param array $context
 */
function patterns_batch_actions($action, $place, $actions_map, &$context) {
      
  patterns_load_components();

  // Nothing to do if there is no action
  if (empty($action)) {
    $context['finished'] = 1;
    return;
  }

  // Start a timer. Since we want each action to be its own http request, we need
  // to ensure the batch api will decide to do it like that by making each action
  // take at least a second to execute
  timer_start('patterns_action');

  // skip action execution if an error is encountered in some of the previous operations
  if (!empty($context['results']['abort'])) return;

  $result = patterns_implement_action($action, $context['results']['identifiers'], $place, $actions_map);

  if (!$result['success']) {
    // we use 'results' to keep track of errors and abort execution if required
    $context['results']['abort'] = TRUE;
    $context['results']['error_message'] = $result['error_message'];
  }

  if (timer_read('patterns_action') < 1000) {
    @usleep(1000 - timer_read('patterns_action'));
  }
}

/**
 * Setup and run an action.
 * 
 * @param $action
 * @param $identifiers
 * @param $place
 * @param $actions_map
 */
function patterns_implement_action($action, &$identifiers, $place = 0, $actions_map = NULL) {

  patterns_set_error_handler();

  $result = array('success' => TRUE);

  ////////////////////////
  // 'form_id': Form_id(s)
  ////////////////////////
  $return = patterns_invoke($action, 'form_id');

  // If prepare removed the data, dont continue with this action
  if (!$action || !$return) {
    return $result;
  }

  if (is_string($return)) {
    $form_ids = array($return);
  }
  elseif ($return) {
    $form_ids = $return;
  }

  ////////////////////////////////////
  // 'actions': Get action description
  ////////////////////////////////////
  $action_descriptions = patterns_invoke($action, 'actions');
  
  // Gather info about the action
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
      patterns_execute_action($form_id, $action_state, $params);

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
      
      ////////////////////
      // CLEAN UP: Let a component cleanup after each action
      ////////////////////
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
  
  // Rebuild the menu
  variable_set('menu_rebuild_needed', TRUE);

  // Get any primary identifiers from the action for further actions to take advantage of
  $id = NULL;
  $id = patterns_invoke($clone, 'identifier', $form_id, $action_state);
  if (isset($id)) {
    $index = isset($clone['action_label']) ? $clone['action_label'] : $place+1;
    $identifiers[$index] = $id;
  }

  patterns_restore_error_handler();

  return $result;
}

/**
 * Execute an action.
 * 
 * @param mixed $form_id the name of function to call
 * @param array $form_state the form_state object
 * @param array $params extra parameters to be passed to the form function
 */
function patterns_execute_action($form_id, &$form_state, $params) {
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

  patterns_executing(TRUE);

  // If we are in batch mode, trick the form api to think
  // otherwise to avoid potential problems
  $batch =& batch_get();
  $batch_clone = $batch;
  $batch = NULL;
  $return = call_user_func_array('drupal_form_submit', $args);
  
  patterns_executing(FALSE);
  $batch = $batch_clone;

}

/**
 * Execute an action.
 * 
 * @deprecated
 * 
 * @param mixed $form_id the name of function to call
 * @param array $form_state the form_state object
 * @param array $params extra parameters to be passed to the form function
 */
function patterns_execute_action_old($form_id, &$form_state, $params) {
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

  patterns_executing(TRUE);

  // If we are in batch mode, trick the form api to think
  // otherwise to avoid potential problems
  $batch =& batch_get();
  $batch_clone = $batch;
  $batch = NULL;

  //$form = call_user_func_array('drupal_retrieve_form', $args);
  //$form['#post'] = $values;
  //$return = drupal_process_form($form_id, $form);
  //dpm($args);

  // drupal_execute fails to keep $form_state in-sync through the
  // whole FAPI process. Issue http://drupal.org/node/346244
  //$return = call_user_func_array('drupal_execute', $args);

  // Fix some parts of the #post values based on the original form
  patterns_sync_form_values($args);

  // Copy of drupal_execute until above issue is fixed
  // TODO: check if this is fixed for drupal_form_submit()
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
  
  patterns_executing(FALSE);
  $batch = $batch_clone;

}

/**
 * Finish a batch operation
 */
function patterns_batch_finish($success, $results, $operations) {
  $info = $_SESSION['patterns_batch_info'];
  if (empty($results['abort'])) {
    foreach ($info as $key => $i) {
      drupal_set_message(t('Pattern "@pattern" ran successfully.', array('@pattern' => $i['title'])));
      // Ste: was
      //db_query("UPDATE {patterns} SET status = 1, enabled = '%s' WHERE pid = %d", time(), $key);
      $query_params = array('en' => time(), 'pid' => $key); // Note: time() != $_SERVER['REQUEST_TIME']
      db_query("UPDATE {patterns} SET status = 1, enabled = :en WHERE pid = :pid", $query_params);
    }
  }
  else {
    $pattern = reset($info);
    drupal_set_message(t('Pattern "@pattern" ran with the errors. Check the error messages to get more details.', array('@pattern' => $pattern['title'])));
    drupal_set_message($results['error_message'], 'error');
  }
  unset($_SESSION['patterns_batch_info']);
  drupal_flush_all_caches();
}

