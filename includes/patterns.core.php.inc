<?php
/*
 * @file
 * A simple, sequential version of running patterns.
 * 
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
      $results = patterns_php_actions($action, $data, $i, $actions_map);
      if (!patterns_error_check_results($results)) {
        drupal_set_message(t('Pattern "@pattern" ran with errors. Check the error messages to get more details.', array('@pattern' => $info['title'])));
      	break;
      }
      else{
        drupal_set_message(t('Pattern "@pattern" ran successfully.', array('@pattern' => $info['title'])));
        $query_params = array('en' => time(), 'pid' => $pid); // Note: time() != $_SERVER['REQUEST_TIME']
        db_query("UPDATE {patterns} SET status = 1, enabled = :en WHERE pid = :pid", $query_params);
      }
      $i++;
    }
  }
  drupal_flush_all_caches();
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
  $identifiers = array();
  return patterns_implement_action($action, $data, $identifiers, $place, $actions_map);
}

