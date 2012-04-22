<?php

/**
 * @file
 * The common functions used by the Batch and PHP running modes.
 */

/**
 * Preparing and validating the action tags as they are written in the pattern
 * file. Concretely, it invokes operations 'prepare', and 'validate' on the
 * pattern component.
 *
 * @param array $actions An array of actions. Each action is an associative
 *   array with keys 'action' and 'data'.
 * @param array $actions_map
 *
 * @return array $results array containing the description of eventual errors
 * @TODO Doc.
 */
function patterns_prepare_action(&$action, &$data, $actions_map) {
//function patterns_prepare_actions(&$actions, $actions_map) {

  $status = PATTERNS_SUCCESS;
  $message = '';

  if (empty($action)) {
    return patterns_results();
  }

  // Keep a list of which modules handle what tags.
  $tag_modules = patterns_tagmodules_get_index($data);

  $errors = array();

  /////////////////////////////////////////////
  // Prepare actions for validation/processing
  /////////////////////////////////////////////
//  foreach ($actions as &$action) {
//    // TODO: error checking.
    $result = patterns_invoke('prepare', $action, $data);
//  }

  // @TODO Not sure if prepare and validate could be merged.
  //       Maybe they were required to be separate when we used to have subpatterns.
//  foreach ($actions as &$action) {
    $key = &$action;
    $data = &$data;

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
      // TODO: Use $index when there are multiple patterns again.
      // $errors[] = t('Action #%num (%tag) in pattern %title: <%tag> is not a valid tag', array('%num' => $index + 1, '%tag' => $data['tag'], '%title' => $pattern_title));
      $errors[] = t('Pattern %title: <%tag> is not a valid tag', array('%tag' => $data['tag'], '%title' => $pattern_title));
    }
    else {
      //////////////////////////////////////////////////
      // Validate tags with their appropriate components
      //////////////////////////////////////////////////
      $results = patterns_invoke('validate', $key, $data);

      if (!patterns_error_check_results($results)) {
        // TODO: Use $index when there are multiple patterns again.
        // $errors[] = t('Action #%num (%tag) in pattern %title: !msg', array('!msg' => $results['msg'], '%num' => $index + 1, '%tag' => $data['tag'], '%title' => $pattern_title));
        $errors[] = t('pattern %title: !msg', array('!msg' => $results['msg'], '%tag' => $data['tag'], '%title' => $pattern_title));
      }
    }
 // }

  if (count($errors)) {
    $message = t('Errors encountered during pre-processing:') . '<br/>' . implode('<br/>', $errors);
    $status = PATTERNS_ERR;
  }

  return patterns_results($status, $message);
}

/**
 * Setup and run an action.
 *
 * @param $action
 * @param $identifiers
 * @param $place
 * @param $actions_map
 *
 * @return
 * @TODO Doc.
 */
function patterns_implement_action($action, $data, &$identifiers, $place = 0, $actions_map = NULL) {

  patterns_set_error_handler();
  // @TODO: I removed the calls to the error handler. Maybe needed again? See how to do it.

  $status = PATTERNS_SUCCESS;
  $msg = '';

  // Refresh the list of forms based on the data we have.
  $tagmodules = patterns_tagmodules_get_index($data);
  $tag_info = patterns_tagmodules_filter($tagmodules, $data['tag']);
  $form_ids = $tag_info[$action];

  /*
   * form_ids is used differently now.
   // If prepare removed the data, don't continue with this action.
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

  if (!patterns_error_check_results($results)) {
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

      if (!patterns_error_check_results($results)) {
        return $results;
      }


      // Auto include FILES
      if (!empty($tag_info[PATTERNS_FILES])) {
        foreach ($tag_info[PATTERNS_FILES] as $file) {
          require_once($file);
        }
      }


      $form_obj = $results['result'];

      // We check for the 'storage' and 'submitted' values in the object to see
      // if it is a form_state instead of form_values. There could be a better way
      // to do this.
      if (array_key_exists('submitted', (array) $form_obj) && array_key_exists('storage', (array) $form_obj)) {
        $action_state = $form_obj;
        $need_buildinfo = FALSE;
      }
      else {

        if (!isset($tag_info['files'])) {
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

      if (!patterns_error_check_results($results)) {
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
        $results['error_message'] = t('Above error(s) occurred while executing action #%num (%action) in %title pattern. Error location(s) are: %errors', array('%num' => $index, '%action' => $action_description, '%title' => $pattern_title, '%errors' => str_replace('][', '->', implode(', ', array_keys($errors)))));
        $results['success'] = FALSE; // @TODO: Do we need success = FALSE ?
        $results['status'] = PATTERNS_ERR;
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

  return patterns_results();
}

/**
 * Execute an action.
 *
 * @param mixed $form_id The name of function to call.
 * @param array $form_state The form_state object.
 * @param array $params Extra parameters to be passed to the form function.
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
  // TODO: check for errors?

  patterns_executing(FALSE);

  //$batch = $batch_clone;

}

/**
 * Group the action and the data fields.
 *
 * @param array $actions The array of actions.
 */
function patterns_reformat_actions(&$actions) {
  $valid_actions = patterns_actions();
  $newactions = array();

  foreach ($actions as $key => $value) {
    //$found = FALSE;
    foreach ($value as $akey => $avalue) {
      if (!array_key_exists($akey, $valid_actions)) {
        // Should not happen.
        drupal_set_message(t("Invalid action: %action.", $akey), 'error');
      }
      else {
          $newactions[] = array('action' => $akey, 'data' => $avalue);
      }
    }
  }
  $actions = $newactions;
}

/**
 * Scan the actions_map array and return info about the file the action was coming
 * from with the following info:
 *
 * - index of the action within the batch_set
 * - title of the pattern
 * - file of the pattern
 *
 * @param mixed $key The key of the action in the actions_map array.
 * @param array $actions_map
 *   Array containing information about of all the actions of the batch_set.
 *
 * @return array $result Concise description of the action.
 */
function patterns_locate_action($key, $actions_map) {
  $result['key'] = $actions_map['map'][$key]['index'];
  $result['title'] = $actions_map['patterns'][$actions_map['map'][$key]['pid']]['title'];
  $result['file'] = $actions_map['patterns'][$actions_map['map'][$key]['pid']]['file'];
  return $result;
}

/**
 *
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
