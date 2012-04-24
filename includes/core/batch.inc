<?php

// Must be included here and not in the .module file
module_load_include('inc', 'patterns', 'includes/core/common');

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

  $include_options = patterns_parser_build_include_options($pid, $params['run-subpatterns']);

  ///////////////////////////////
  // Looping through the sections
  ///////////////////////////////
  $tot_sec = count($sections);
  $cur_sec = 1;
  foreach ($sections as $section => $actions) {
    // by default this pattern should run included ?
    // this setting can be overwritten by the pattern file itself

    $actions = patterns_parser_retrieve_actions_from_section($actions, $include_options);

    // This can happen if includes are disabled
    if (count($actions) == 0 ) {
      drupal_set_message(t('Section "%section" did not contain any valid action. Please check your policy for included patterns.', array('%section' => $section)), 'warning');
      continue;
    }

    // Reformat the actions. $actions passed as reference.
    patterns_reformat_actions($actions);

    $progress_params = array(
                          '%section' => $section,
                          '%cur_sec' => $cur_sec,
                          '%tot_sec' => $tot_sec,
    );

    $progress = t('Running section "%section" (%cur_sec/%tot_sec), action @current out of @total', $progress_params);

    $batch = array(
      //'title' => t('Processing section %section of pattern %pattern', array('%section' => $section, '%pattern' => $info['title'])),
      'title' => t('Processing pattern %pattern', array('%pattern' => $info['title'])),
      'progress_message' => $progress,
      'operations' => array(),
      'file' =>  drupal_get_path('module', 'patterns') . '/includes/core/batch.inc',
      'finished' => 'patterns_batch_finish',
      'init_message' => t('Initializing section "%section" (%cur_sec/%tot_sec)', array(
                          '%section' => $section,
                          '%cur_sec' => $cur_sec,
                          '%tot_sec' => $tot_sec,
                        )),
    );

    $total = count($actions);
    $i=0;
    foreach ($actions as $action) {
      $batch['operations'][] = array('patterns_batch_action', array($action['action'],
                                                                    $action['data'],
                                                                    $i,
                                                                    $actions_map,
                                                                    $section,
                                                                    ));
      $i++;
    }

    $_SESSION['patterns_batch_info'] = $infos;
    batch_set($batch);
    $cur_sec++;
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
function patterns_batch_action($action, $data, $place, $actions_map, $section, &$context) {

  // Since we are in the batch, we need to load some things once more.
  module_load_include('inc', 'patterns', 'includes/core/common');
  module_load_include('inc', 'patterns', 'includes/core/modules');
  module_load_include('inc', 'patterns', 'includes/core/token');
  patterns_io_load_components();

  // Nothing to do if there is no action
  if (empty($data) || empty($action)) {
    $context['finished'] = 1;
    drupal_set_message(t('Cannot execute empty action.'), 'error');
    return FALSE;
  }

  // Saving the name of the section in the context
  $context['results']['section'] = $section;

  // Start a timer. Since we want each action to be its own http request, we need
  // to ensure the batch api will decide to do it like that by making each action
  // take at least a second to execute
  timer_start('patterns_action');

  /////////////////////////
  // PREPARATION
  $results = patterns_prepare_action($action, $data, $actions_map);

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
  $section = $results['section'];
  if (empty($results['abort'])) {
    foreach ($info as $key => $i) {
      drupal_set_message(t('Section "@section" of pattern "@pattern" ran successfully.', array('@pattern' => $i['title'],
                                                                                             '@section' => $section,
      )));
      $query_params = array(':time' => time(), // Note: time() != $_SERVER['REQUEST_TIME']
                            ':pid' => $key,
                            ':en' => PATTERNS_STATUS_ENABLED,
      );
      db_query("UPDATE {patterns} SET status = :en, enabled = :time WHERE pid = :pid", $query_params);
    }
  }
  else {
    $pattern = reset($info);
    drupal_set_message(t('Section "@section" of pattern "@pattern" ran with errors. Check the error messages to get more details.',
      array('@pattern' => $pattern['title'], '@section' => $section, )
      ),
     'error');
    drupal_set_message($results['error_message'], 'error');
  }

  unset($_SESSION['patterns_batch_info']);
  drupal_flush_all_caches();
}
