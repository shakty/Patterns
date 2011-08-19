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
 * @param boolean $quickrun flag to check if the patterns exists in the database
 * 
 */
function patterns_execute_pattern($pattern, $params = array()) {

  drupal_set_time_limit(0);


  if (!isset($params['quickrun']) || !$params['quickrun']) {
    if (!is_object($pattern)) {
      $pattern = patterns_get_pattern($pattern);
  
      if (!$pattern) {
        return FALSE;
      }
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
  
// TODO: Patterns details was returning this in case of recursive includes  
//  $result['actions_map'][] = array(
//          'pid'   => $pattern->pid,
//          'index' => $key,
//        );
  
  $actions_map = array('patterns' => $infos, 'map' => NULL); // Fix this
  ////////////////////////////////////////
  
  
  // If there are no actions or modules, most likely the pattern
  // was not created correctly.
  if (empty($sections)) {
    drupal_set_message(t('Could not recognize the format of pattern %title, aborting.', array('%title' => $info['title'])), 'error');
    return FALSE;
  }
  
  if (!empty($modules)) {
    // Install modules, if necessary
    $results = patterns_install_modules($modules);
    if (!$results['success']) {
      drupal_set_message($results['error_message'], 'error');
      return FALSE;
    }
  }
  
  ///////////////////////////////
  // Looping through the sections
  ///////////////////////////////
    
  foreach ($sections as $section => $actions) {
    // $actions passed as reference
    $results = patterns_prepare_actions($actions, $actions_map);
    
    if (!patterns_error_check_results($results)){
      return $results;
    }
    
    $batch = array(
      'title' => t('Processing section %section of pattern %pattern', array('%section' => $section, '%pattern' => $info['title'])),
      'progress_message' => t('Running action @current out of @total'),
      'operations' => array(),
      'file' => drupal_get_path('module', 'patterns') . '/includes/patterns.core.batch.inc',
      'finished' => 'patterns_batch_finish'
    );
  
    $i = 0;
    $total = count($actions);
    foreach ($actions as $action => $data) {
      $batch['operations'][] = array('patterns_batch_actions', array($action, $data, $i, $actions_map));
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
 * @return array $results array containing the description of eventual errors
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
    patterns_invoke('prepare', $key, $data);
  }

  // @TODO: do we need two foreach?
  foreach ($actions as $key => &$data) {    
    
/////////////////////////////////////////////////////////
// This is in case of included patterns. Not for now    
    
//    $action_location = patterns_locate_action ($data['tag'], $actions_map);
//    $index = $action_location['key'];
//    $pattern_title = $action_location['title'];
//    $pattern_file = $action_location['file'];
/////////////////////////////////////////////////////////

    // TODO: manage multiple pattern includes
    $pattern_info = reset($actions_map['patterns']);
    $pattern_title = $pattern_info['title'];
    
    if (!array_key_exists($data['tag'], $tag_modules)) {
    	// TODO: $index is undefined.
      $errors[] = t('Action #%num (%tag) in pattern %title: <%tag> is not a valid tag', array('%num' => $index+1, '%tag' => $data['tag'], '%title' => $pattern_title));
    }
    else {
      //////////////////////////////////////////////////
      // Validate tags with their appropriate components
      //////////////////////////////////////////////////
      $results = patterns_invoke('validate', $key, $data);
      
      if (!patterns_error_check_results($results)) {
      	// TODO: $index is undefined.
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
function patterns_batch_actions($action, $data, $place, $actions_map, &$context) {
      
  patterns_load_components();

  // Nothing to do if there is no action
  if (empty($data) || empty($action)) {
    $context['finished'] = 1;
    drupal_set_message('Cannot execute empty action.','error');
    return FALSE;
  }

  // Start a timer. Since we want each action to be its own http request, we need
  // to ensure the batch api will decide to do it like that by making each action
  // take at least a second to execute
  timer_start('patterns_action');

  // skip action execution if an error is encountered in some of the previous operations
  if (!empty($context['results']['abort'])) return;

  $results = patterns_implement_action($action, $data, $context['results']['identifiers'], $place, $actions_map);

  if ($results['status'] == PATTERNS_ERR) {
    // we use 'results' to keep track of errors and abort execution if required
    $context['results']['abort'] = TRUE;
    $context['results']['error_message'] = $results['msg'];
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
function patterns_implement_action($action, $data, &$identifiers, $place = 0, $actions_map = NULL) {
  
  patterns_set_error_handler();
  // @TODO: I removed the calls to the error handler. Maybe needed again? See how to do it.
  
  $status = PATTERNS_SUCCESS;
  $msg = '';
  
  $tag_info = patterns_get_tagmodules_index($data['tag']);
  $form_ids = $tag_info[$action];
  
  /*
   * form_ids is used differently now.
  // If prepare removed the data, dont continue with this action
  if (empty($form_ids)) {
    drupal_set_message(t('No Form ID to execute was found'), 'error');
    return patterns_results(PATTERNS_ERR, t('No Form ID to execute was found'));
  }
  */

  // Gather info about the action
  $action_location = patterns_locate_action($place, $actions_map);
  $index = $action_location['key'] + 1;
  $pattern_title = $action_location['title'];
  $pattern_file = $action_location['file'];

  // See which forms to execute sequentially. This is similar to what used to be called 'form_ids'.
  $results = patterns_invoke('callbacks', $action, $data);
  
  if (!patterns_error_check_results($results)){
    return $results;
  }

  $funcs = $results['result'];

  // Build the action
  foreach ($funcs as $func) {
    if (!in_array($func, $form_ids)) { // Run it as a custom callback function.
      $result = $func($action, $data); // TODO: error checking
  		if (isset($result['msg'])) {
        drupal_set_message($result['msg']);
  		}
  	}
  	else {
  
    	$form_id = $func;
      $clone = $data; // TODO: does this prevent subsequent form_ids' communication?
      
      $results['action_descriptions'][$place][] = $tag_info['descr'];
  
      // If tokens are enabled, apply tokens to the action values
      // before processing.
      if (module_exists('token')) {
        _patterns_recurse_tokens($clone, $identifiers);
        //array_walk($clone, '_patterns_replace_tokens', $identifiers);
      }
  
      /////////////////////////////////////////////////////
      // BUILD: Get the form data for the action. This can return either 
      // just the form values, or the full form_state object.
      /////////////////////////////////////////////////////
      $results = patterns_invoke('build', $action, $clone, $form_id);
   
  	  if (!patterns_error_check_results($results)){
        return $results;
      }
  
      $form_obj = $results['result'];
      
    	// We check for the 'storage' and 'submitted' values in the object to see
    	// if it is a form_state instead of form_values. There could be a better way
    	// to do this.
    	if (array_key_exists('submitted', (array)$form_obj) && array_key_exists('storage', (array)$form_obj)) {
    		$action_state = $form_obj;
    		$need_buildinfo = FALSE;
    	}
    	else {
  
    		if(!isset($tag_info['files'])) {
    			$files = array();
    		}
    		else {
    		  $files = $tag_info['files'];	
    		}

    		$action_state = array(
    		      // @TODO Ste: check if storage is needed here
              'storage' => NULL,
              'submitted' => FALSE,
              'build_info' => array('files' => $files), 
              'values' => $form_obj,
    		);
    		$need_buildinfo = TRUE;
    	}

      ////////////////////////////////////////////////////
      // Get any extra parameters required for the action
      ////////////////////////////////////////////////////
      $results = patterns_invoke('params', $action, $clone, $form_id, $action_state);

  	  if (!patterns_error_check_results($results)){
        return $results;
      }
  
      // A single, simple value can be returned as a parameter, which is then
      // put into an array here.
      if (!isset($results['result'])) {
      	$results['result'] = NULL;
      }
      if (!is_array($results['result'])) {
        $params = array($results['result']);
      }
      else {
        $params = $results['result'];
      }
  
      if ($need_buildinfo) {
        $action_state['build_info']['args'] = $params;
      }
      
      ///////////////////
      // Execute action: pass action_state and params to a form
      ///////////////////
      patterns_execute_action($form_id, $action_state, $params);

      // @TODO: do we need this?
      if ($errors = form_get_errors()) {
        $results['error_message'] = t('Above error(s) occured while executing action #%num (%action) in %title pattern. Error location(s) are: %errors', array('%num' => $index, '%action' => $action_description, '%title' => $pattern_title, '%errors' => str_replace('][', '->', implode(', ', array_keys($errors)))));
        $results['success'] = FALSE;
        return $results;   
      }
        
      ////////////////////
      // CLEAN UP: Let a component cleanup after each action
      ////////////////////
      $results = patterns_invoke('cleanup', $action, $clone, $form_id, $action_state);
      
      // Do not return here, just print on screen
      patterns_error_check_results($results);     
     }
    } // End: Form_id execution
    
    // Clear the cache in case it causes problems
  cache_clear_all();
  
  // STE: This should be done in the cleanup, if the module has changed the menus
  // Rebuild the menu
  //variable_set('menu_rebuild_needed', TRUE);

  // TODO:  Check if identifiers are needed
  
  // Get any primary identifiers from the action for further actions to take advantage of
  //  $id = NULL;
  //  $id = patterns_invoke($clone, 'identifier', $form_id, $action_state);
  //  
  //  if (isset($id)) {
  //    $index = isset($clone['action_label']) ? $clone['action_label'] : $place+1;
  //    $identifiers[$index] = $id;
  //  }

  patterns_restore_error_handler();

  return $results;
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
  //$batch =& batch_get();
  //$batch_clone = $batch;
  //$batch = NULL;
  $return = call_user_func_array('drupal_form_submit', $args);
  
  patterns_executing(FALSE);
  
  //$batch = $batch_clone;

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

