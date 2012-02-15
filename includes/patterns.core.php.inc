<?php

// Must be included here and not in the .module file
module_load_include('inc', 'patterns', 'includes/patterns.core.common');
 
/**
 * @file
 * A simple, sequential version of running patterns.
 */

/**
 * Execute a Pattern. Actions will be called sequentially.
 *
 * E.g.: loading additional modules, and creating the array of patterns actions.
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
function patterns_execute_pattern_php($pattern, $params, $patterns_details, $actions_map) {
  $pattern_details = current($patterns_details);
  $pid = key($patterns_details);

  $info = $pattern_details['info'];
  $infos = array($pid => $info);

  $modules = $pattern_details['modules'];
  $sections = $pattern_details['sections'];
  ///////////////////////////////
  // Looping through the sections
  ///////////////////////////////

  foreach ($sections as $section => $actions) { // TODO: is this loop necessary?
    // Reformat the actions. $actions passed as reference.
    patterns_reformat($actions);
    // $actions passed as reference.
    $results = patterns_prepare_actions($actions, $actions_map);

    if (!patterns_error_check_results($results)) {
      return $results;
    }
    $i = 0;
    $success = TRUE;
    foreach ($actions as $action) {
      $results = patterns_php_actions($action['action'], $action['data'], $i, $actions_map);
      if (!patterns_error_check_results($results)) {
        drupal_set_message(t('Pattern "@pattern" ran with errors. Check the error messages to get more details.', array('@pattern' => $info['title'])));
        $success = FALSE;
        break;
      }
      $i++;
    }
    if ($success) {
      drupal_set_message(t('Pattern "@pattern" ran successfully.', array('@pattern' => $info['title'])));
      $query_params = array('en' => time(), 'pid' => $pid); // Note: time() != $_SERVER['REQUEST_TIME']
      db_query("UPDATE {patterns} SET status = 1, enabled = :en WHERE pid = :pid", $query_params);
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
 * @param array $action
 * @param mixed $place index of the current operation within the batch_set
 * @param array $actions_map [$pid pattern id, $index ??]
 * @TODO Doc.
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
