<?php
/*
 * @file
 * A simple, sequential version of running patterns.
 * 
 * @TODO: Organize, reuse functions better.
 */
// TODO: move most of this to common
/**
 * 
 * Performs all the pre-operations, before doing anything serious
 * 
 * @param $pattern
 * @param $params
 */
function patterns_execute_pattern($pattern, $params = array()) {
  module_load_include('inc', 'patterns', 'includes/patterns.core.common');
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
  $infos = array($pid => $info);
  
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

  // If there are no sections, most likely the pattern was not created correctly.
  if (empty($sections)) {
    drupal_set_message(t('Could not recognize the format of pattern %title, aborting.', array('%title' => $info['title'])), 'error');
    return FALSE;
  }

  // Install modules, if necessary
  $results = patterns_install_modules($modules);
  if (!$results['success']) {
    drupal_set_message($results['error_message'], 'error');
    return FALSE;
  }

  ///////////////////////////////
  // Looping through the sections
  ///////////////////////////////

  foreach ($sections as $section => $actions) { // TODO: is this loop necessary?
    // $actions passed as reference
    $results = patterns_prepare_actions($actions, $actions_map);
    
    if (!patterns_error_check_results($results)){
      return $results;
    }
    $i = 0;
    foreach ($actions as $action => $data) {
      patterns_php_actions($action, $data, $i, $actions_map);
      $i++;
    }
/*
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
    */
  }
 
  // @TODO: return false in case of errors
  return TRUE;
}



/**
 * 
 * Execute a single pattern action.
 * 
 * @TODO improve documentation
 * 
 * 
 * @param array $action
 * @param mixed $place index of the current operation within the batch_set
 * @param array $actions_map [$pid pattern id, $index ??]
 */
function patterns_php_actions($action, $data, $place, $actions_map) {

  patterns_load_components(); // TODO: move this out of here?

  // Nothing to do if there is no action
  if (empty($data) || empty($action)) {
    drupal_set_message(t('Cannot execute empty action.'), 'error');
    return FALSE;
  }
  $result = patterns_implement_action($action, $data, $place, NULL, $actions_map);

  return patterns_results($status, $message);
}


// TODO: remove this function
/**
 * 
 * Performs all the operation necessary to build a correct form_state (op: 'validate', 'build', 'params')
 * Calls patterns_execute_action() which actually builds and processes the form.
 * 
 * @param unknown_type $action
 * @param unknown_type $place
 * @param unknown_type $actions_map
 */
function patterns_implement_action_php($action, $data, $place = 0, $actions_map = NULL) {

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


// TODO: remove this function
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

