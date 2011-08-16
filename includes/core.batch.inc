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
  
  /////////////////////////////////////////
  // Get Patterns details
  $patterns_details = patterns_get_pattern_details($pattern);

  // TODO: handle multiple patterns correctly
  // Let us assume we have only one pattern in the returned array of details
  $pid = key($patterns_details);
  $pattern_details = current($patterns_details); 
  $info = $pattern_details['info'];
  
  // Array of infos of multiple patterns (we have only one for now)
  $infos = array($pid=>$info);
  
  $modules = $pattern_details['modules'];
  $sections = $pattern_details['sections'];
  
  // Ste: Actions_map seems to be used with includes. Probably we do not need it now.
  // $actions_map = array('patterns' => $pattern_details['info'], 'map' => $pattern_details['actions_map']);
  $actions_map = array('patterns' => $infos, 'map' => NULL);
  ////////////////////////////////////////
  
  
  // If there are no actions or modules, most likely the pattern
  // was not created correctly.
  if (empty($sections)) {
    drupal_set_message(t('Could not recognize the format of pattern %title, aborting.', array('%title' => $info['title'])), 'error');
    return FALSE;
  }
  
  if (!empty($modules)) {
    // Install modules, if necessary
    $result = patterns_install_modules($modules);
    if (!$result['success']) {
      drupal_set_message($result['error_message'], 'error');
      return FALSE;
    }
  }
  
  ///////////////////////////////
  // Looping through the sections
  ///////////////////////////////
    
  foreach ($sections as $section => $actions) {
    // $actions passed as reference
    $result = patterns_prepare_actions($actions, $actions_map);
    
    if (!$result['status'] == PATTERNS_ERR) {
      drupal_set_message($result['msg'], 'error');
      return FALSE;
    }
    else if (!$result['status'] == PATTERNS_WARN) {
      drupal_set_message($result['msg'], 'warning');
    }
    
    $batch = array(
      'title' => t('Processing section %section of pattern %pattern', array('%section' => $section, '%pattern' => $info['title'])),
      'progress_message' => t('Running action @current out of @total'),
      'operations' => array(),
      'file' => drupal_get_path('module', 'patterns') . '/includes/core.batch.inc',
      'finished' => 'patterns_batch_finish'
    );
  
    $i = 0;
    $total = count($actions);
    foreach ($actions as $action) {
      $batch['operations'][] = array('patterns_batch_actions', array($action, $i, $actions_map));
      $i++;
    }
    
    $_SESSION['patterns_batch_info'] = $infos;
  
    batch_set($batch); 
  }
  
  return TRUE;
}

/**
 * Preparing and prevalidating the action tags as they are written 
 * in the pattern file. Concretely, it invokes operations 'prepare', 
 * and 'prevalidate' on the pattern component. 
 * 
 * @param array $actions
 * @param array $actions_map
 * @return array $result array containing the description of eventual errors
 */
function patterns_prepare_actions(&$actions, $actions_map) {
  
  $status = PATTERNS_SUCCESS;
  $message = '';
  
  if (empty($actions)) return patterns_results();
  
  patterns_load_components();

  // Keep a list of which modules handle what tags
  $tag_modules = patterns_get_tagmodules_index();

  $errors = array();
  
  /////////////////////////////////////////////
  // Prepare actions for validation/processing
  /////////////////////////////////////////////
  foreach ($actions as $key => &$data) {
    patterns_invoke('prepare', $key, &$data);
  }

  // @TODO: do we need two foreach?
  foreach ($actions as $key => &$data) {    
/////////////////////////////////////////////////////////
// This is in case of included patterns. Not for now    
    
//    $action_location = patterns_locate_action($key, $actions_map);
//    $index = $action_location['key'];
//    $pattern_title = $action_location['title'];
//    $pattern_file = $action_location['file'];

/////////////////////////////////////////////////////////
    
    if (!array_key_exists($data['tag'], $tag_modules)) {
      $errors[] = t('Action #%num (%tag) in pattern %title: <%tag> is not a valid tag', array('%num' => $index+1, '%tag' => $data['tag'], '%title' => $pattern_title));
    }
    else {
      //////////////////////////////////////////////////
      // Validate tags with their appropriate components
      //////////////////////////////////////////////////
      $results = patterns_invoke('validate', $key, &$data);
      if ($results['status'] == PATTERNS_ERR) {
        $errors[] = t('Action #%num (%tag) in pattern %title: !msg', array('!msg' => $error, '%num' => $index+1, '%tag' => $data['tag'], '%title' => $pattern_title));
      }
    }
  }

  if (count($errors)) {
    $message = t('Errors encountered during pre-processing:') . '<br/>' . implode('<br/>', $errors);
    $status = PATTERNS_ERR;
  }

  return patterns_results($status,$message);
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

  // PRE-LUNCH
  // HERE MODIFY missing hooks and other stuff
  
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

    // NOTE: In some versions, the params block was before these action_state-setting blocks. Try to change them if bugs arise.
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
    $need_buildinfo = FALSE;
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
            // @TODO Change the file passed to the form
            $files = array('modules/user/user.admin.inc',
                           'modules/field/field_ui.admin.inc',
            );
    		$action_state = array(
    		      // @TODO Ste: check if storage is needed here
              'storage' => NULL,
              'submitted' => FALSE,
              'build_info' => array('args' => $params, 'files' => $files), 
              'values' => $form_obj
    		);
    		$need_buildinfo = TRUE;
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

    if ($need_buildinfo) {
      $action_state['build_info'] = array('args' => $params, 'files' => $files);
    }
      ///////////////////
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
 * Finish a batch operation
 */
function patterns_batch_finish($success, $results, $operations) {
  $info = $_SESSION['patterns_batch_info'];
  if (empty($results['abort'])) {
    
    // TODO: info is different: was pid-> array of infos
    
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

