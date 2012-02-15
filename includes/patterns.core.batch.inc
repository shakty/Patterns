<?php

// Must be included here and not in the .module file
module_load_include('inc', 'patterns', 'includes/patterns.core.common');
 
/**
 * @file
 * Running patterns using Drupal's batch API.
 */

/**
 * Starts preliminary operations for pattern execution.
 *
 * E.g.: loading additional modules, and creating the array of patterns actions.
 *
 * If there are no errors, it creates the batch array of operations. Each of them
 * is a call to patterns_batch_actions with different parameters.
 *
 * @param stdClass $pattern Pattern object as loaded by patterns_get_pattern().
 * @param array $params Parameters for executing the pattern. Array format as follows:
 * - pid => Pid of the pattern as it is in the database
 * - run-subpatterns => ['first-update', always', 'update', 'first', 'never']
 * - quickrun => boolean flag to indicate that the execution was initiated from a quickrun
 *   and the pattern is already loaded into $pattern.
 * @param $patterns_details
 * @param $actions_map
 *
 * @return
 * @TODO Doc.
 */
function patterns_execute_pattern_batch($pattern, $params, $patterns_details, $actions_map) {
  $pattern_details = current($patterns_details);
  $pid = key($patterns_details);

  $info = $pattern_details['info'];
  $infos = array($pid => $info);

  $modules = $pattern_details['modules'];
  $sections = $pattern_details['sections'];

  ///////////////////////////////
  // Looping through the sections
  ///////////////////////////////

  foreach ($sections as $section => $actions) {
    // Reformat the actions. $actions passed as reference.
    patterns_reformat($actions);

//    //$actions passed as reference.
//    $results = patterns_prepare_actions($actions, $actions_map);
//
//    if (!patterns_error_check_results($results)) {
//      return $results;
//    }

    $batch = array(
      'title' => t('Processing section %section of pattern %pattern', array('%section' => $section, '%pattern' => $info['title'])),
      'progress_message' => t('Running action @current out of @total'),
      'operations' => array(),
      'file' =>  drupal_get_path('module', 'patterns') . '/includes/patterns.core.batch.inc',
      'finished' => 'patterns_batch_finish'
    );

    //$batch['operations'][] = array('patterns_prepare_actions', array($actions, $actions_map));
    $i = 0;
    $total = count($actions);
    foreach ($actions as $action) {
      $batch['operations'][] = array('patterns_batch_actions', array($action['action'], $action['data'], $i, $actions_map));
      $i++;
    }

    $_SESSION['patterns_batch_info'] = $infos;

    batch_set($batch);
  }

  return TRUE;
}

/**
 *
 * Executes a batch action.
 *
 * @param array $action
 * @param mixed $place index of the current operation within the batch_set
 * @param array $actions_map [$pid pattern id, $index ??]
 * @param array $context
 * @TODO Doc.
 */
function patterns_batch_actions($action, $data, $place, $actions_map, &$context) {

  // Since we are in the batch, we need to load some things once more.
  module_load_include('inc', 'patterns', 'includes/patterns.core.common');
  patterns_load_components();

  // Nothing to do if there is no action
  if (empty($data) || empty($action)) {
    $context['finished'] = 1;
    drupal_set_message(t('Cannot execute empty action.'), 'error');
    return FALSE;
  }
  
  // Start a timer. Since we want each action to be its own http request, we need
  // to ensure the batch api will decide to do it like that by making each action
  // take at least a second to execute
  timer_start('patterns_action');

  /////////////////////////
  // PREPARATION
  $results = patterns_prepare_actions($action, $data, $actions_map);

  if (!patterns_error_check_results($results)) {
    $context['finished'] = 1;
    drupal_set_message(t('An error occurred while preparing the actions.'), 'error');
    return FALSE;
  }
  ///////////////////////
  
  
  // skip action execution if an error is encountered in some of the previous operations
  if (!empty($context['results']['abort'])) {
    return;
  }

  $results = patterns_implement_action($action, $data, $context['results']['identifiers'], $place, $actions_map);

  if ($results['status'] == PATTERNS_ERR) { // TODO: handle better?
    // we use 'results' to keep track of errors and abort execution if required
    $context['results']['abort'] = TRUE;
    $context['results']['error_message'] = $results['msg'];
  }

  if (timer_read('patterns_action') < 1000) {
    @usleep(1000 - timer_read('patterns_action'));
  }
}

/**
 * Finishes a batch operation.
 * @TODO Doc.
 */
function patterns_batch_finish($success, $results, $operations) {
  $info = $_SESSION['patterns_batch_info'];
  if (empty($results['abort'])) {

    // TODO: info is different: was pid-> array of infos

    foreach ($info as $key => $i) {
      drupal_set_message(t('Pattern "@pattern" ran successfully.', array('@pattern' => $i['title'])));
      $query_params = array('en' => time(), 'pid' => $key); // Note: time() != $_SERVER['REQUEST_TIME']
      db_query("UPDATE {patterns} SET status = 1, enabled = :en WHERE pid = :pid", $query_params);
    }
  }
  else {
    $pattern = reset($info);
    drupal_set_message(t('Pattern "@pattern" ran with errors. Check the error messages to get more details.', array('@pattern' => $pattern['title'])));
    drupal_set_message($results['error_message'], 'error');
  }
  // TODO: check 
  //unset($_SESSION['patterns_batch_info']);
  drupal_flush_all_caches();
}
