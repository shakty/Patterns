<?php
/**
 * @file
 * Drush Patterns module commands
 */

/**
 * Implements hook_drush_help().
 */
function patterns_drush_help($section) {
  switch ($section) {
    case 'drush:patterns-run':
      /** @todo Pattern files are searched for in the following locations: sites/[site name]/patterns, profiles/default/patterns, sites/all/patterns, sites/all/modules/contrib/patterns/patterns
       --do we want to support this?*/
      return dt('Import and enable a pattern file.');
      break;
    case 'drush:patterns-enable':
      return dt('Enable a pattern by name or id');
      break;
    case 'drush:patterns-import':
      return dt('Import a pattern file.');
      break;
    case 'drush:patterns-list':
      return dt("List all available patterns.");
      break;
    case 'drush:patterns-info':
      return dt("Show all info on a given pattern.");
      break;
  }
}

/**
 * Implements hook_drush_command().
 */
function patterns_drush_command() {

  $items['patterns-enable'] = array(
    'callback' => 'drush_patterns_enable',
    'description' => 'Enable the specified pattern name',
    'examples' => array(
      'drush patterns-enable pattern_name [first-update|always|update|first|never]' => 'Enable the pattern specified by pattern id or pattern name'
    ),
    'arguments' => array(
      'id' => 'A pattern id or name',
    ),
    'drupal dependencies' => array('patterns', 'token'),
    'core' => array('7'),
  );
  $items['patterns-import'] = array(
    'callback' => 'drush_patterns_import',
    'description' => 'Import the specified pattern file.',
    'examples' => array(
      'drush patterns-import path/to/patterns/file.[xml/yaml]' =>
        'Import the specified pattern file.',
      'drush patterns import path/to/pattern/file.[xml/yaml] pattern_name' => 'Import the specified pattern file and give it a machine readable name'
    ),
    'arguments' => array(
      'pattern_file' => 'The path to a pattern file, beginning from the Drupal base directory',
      'pattern_name' => 'Enter a pattern name if you wish to override the title field entered in a pattern.'
    ),
    'drupal dependencies' => array('patterns', 'token'),
    'core' => array('7'),
  );
  $items['patterns-run'] = array(
    'callback' => 'drush_patterns_run',
    'description' => 'Import and enable the specified pattern file',
    'examples' => array(
      'drush patterns-run path/to/patterns/file.xml' =>
        'Import and enable the specified pattern file',
      'drush patterns-run path/to/pattern/file.xml pattern_name' => 'Import and enable the specified pattern file and give it a machine readable name'
    ),
    'arguments' => array(
      'pattern_file' => 'The path to a pattern file',
      'pattern_name' => 'An optional machine-readable pattern name to override the one automatically generated from the title field of the pattern'
    ),
    'drupal dependencies' => array('patterns', 'token'),
    'core' => array('7'),
  );
  $items['patterns-list'] = array(
    'callback' => 'drush_patterns_list',
    'description' => "List all available patterns."
  );
  $items['patterns-info'] = array(
    'callback' => 'drush_patterns_info',
    'description' => "Show all info on a given pattern.",
    'arguments' => array(
      'id' => "The pattern id or name of the pattern for which to show info.",
    )
  );

  //NOT CLEANED YET
  $items['patterns-form-helper'] = array(
    'description' => 'Enable/disable patterns form helper',
    'arguments' => array(
      'action' => "Valid values: enable, disable.",
    )
  );
  $items['patterns-allow-publish'] = array(
    'description' => 'Enable/disable patterns publishing',
    'arguments' => array(
      'action' => "Valid values: enable, disable.",
    )
  );
  $items['patterns-paths'] = array(
    'description' => "List all paths patterns will be looked for",
  );
  $items['patterns-publish'] = array(
    'description' => "Publish a pattern.",
    'arguments' => array(
      'pid' => "The ID of the pattern to publish.",
    )
  );
  $items['patterns-unpublish'] = array(
    'description' => "Unpublish a pattern.",
    'arguments' => array(
      'pid' => "The ID of the pattern to unpublish.",
    )
  );
  return $items;
}

/**
 * Imports, Enables, and Runs the specified pattern file
 * @param string pattern file pathname
 * @param string optional machine readable pattern name
 */
function drush_patterns_run($pattern_file, $pattern_name = '') {
  /** Since I want to be able to specify a file name in my interface, in
   * order to use the patterns_get_pattern() logic as is, I need to
   * standardize on pattern name, and fire off this "redundant" query to do
   * so. I'm trading off efficiency for ensuring future compatibility with
   * patterns module.*/
  $name = drush_db_result(db_query("SELECT name FROM {patterns} WHERE `file` LIKE '%s'", "%$pattern_file"));

  $result = FALSE;
  $subpattern_mode = 'always'; //is this right?
  if (!$name) {

    // we should also be allowed to specify an existing pattern by name
    $pattern = patterns_get_pattern($pattern_file);
    $pattern_file = $pattern->file;
    $name = drush_db_result(db_query("SELECT name FROM {patterns} WHERE `file` LIKE '%s'", "%$pattern_file"));

    if (!$name) {
      if (drush_confirm(dt("Pattern '$pattern_file' has not been imported. Do you want to import it now? "))) {
        $name = drush_patterns_import($pattern_file, $pattern_name);
      }
      else {
        drush_print(dt('patterns-run has been cancelled.'));
        return FALSE;
      }
    }
  }

  // makes sure the most recent version is loaded. who wants an old pattern?
  $patterns = patterns_io_get_patterns();

  if ($name) {
    $result = drush_patterns_enable($name, $subpattern_mode);
  }

  if (!$result) {
    drush_set_error(dt("Failed to run pattern '$name' from pattern file '$pattern_file'"));
    return FALSE;
  }
  return TRUE;
}

/**
 * Imports the specified patterns file
 * @param string path to the patterns file
 * @param string optional machine readable name for the pattern you are importing
 * @return mixed bool|string FALSE upon failure; the name of the imported
 * pattern upon success
 */
function drush_patterns_import($pattern_file, $pattern_name = '') {
  static $patterns_dir = NULL;

  drush_print(dt("Importing new pattern file '$pattern_file'"));

  if (!file_exists($pattern_file)) {
    drush_set_error(dt("File '$pattern_file' does not exist."));
    return FALSE;
  }
  $form_state = array();
  $form_state['values'] = array();
  $pattern_path_info = pathinfo($pattern_file);
  $result = TRUE;
  $err_msg = '';
  if ($pattern_path_info['extension'] == 'yaml') {
    drush_set_error(dt('Import feature currently supports only XML file format.'), 'warning');
  }
  else { //assume xml
    $xml = file_get_contents($pattern_file);
    if (!$xml) {
      drush_set_error(dt("XML pattern file '$pattern_file' is unreadable."));
      $result = FALSE;
    }
  }
  if (!$result) {
    drush_set_error(dt("Failed to import '$pattern_file'."));
    return FALSE;
  }
  // no pretty support in simplexml_..., so using DOMDocument
  $xmldoc = new DOMDocument();
  $xmldoc->formatOutput = TRUE;
  $xmldoc->loadXML($xml);
  $pattern_source = $xmldoc->saveXML();

  if (empty($pattern_name)) {
    $xpath = new DOMXpath($xmldoc);
    $query = 'info/title';
    $entries = $xpath->query($query); //returns DOMNodeList
    foreach ($entries as $entry) {
      $pattern_name = $entry->textContent;
      break;
    }
  }
  /**
   * Make sure pattern name is machine readable
   *
   * would rather do this all in one go, but I can't seem to make the order
   * of the replacements work right.
   */
  $pattern_name = preg_replace(array('/^_/', '/[^a-zA-Z0-9\s_]+/'), '', $pattern_name);
  $pattern_name = preg_replace('/\s+/', '_', $pattern_name);
  // Mimic the form in patterns_import_source.
  $form_state['post'] = TRUE;
  $form_state['values']['xmlsource'] = $pattern_source;
  $form_state['values']['xmlname'] = $pattern_name;
  $return = $pattern_name;
  //Ok. so let's use the patterns API for executing this ad hoc form.
  patterns_execute_action('patterns_import_source', $form_state, array());
  $errors = form_get_errors();
  if ($errors) {
    $output = '';
    while (list($field, $err) = each($errors)) {
      $output .= "[$field]:\t$err\n";
    }
    drush_set_error(dt($output));
    $return = FALSE;
  }
  $msgs = drupal_get_messages();
  if ($msgs) {
    $output = '';
    while (list($type, $ms) = each($msgs)) {
      $output .= "[$type]:\n\t" . implode("\n", $ms) . "\n\n";
    }
    drush_print(dt($output));
  }
  //give back the name of the pattern upon success, FALSE on errors
  return $return;
}

/**
 * Enables the specified pattern
 * @param string $id pattern name or pattern id
 * @param string $subpattern_mode Choice of first-update, always, update,
 * first, never. Default is first-update.
 *
 * @todo Currently the function will barge through and run all sub-patterns by
 * default. will want to revisit this and perhaps implement a second parameter
 * to handle whether to do so or not.
 */
function drush_patterns_enable($id, $subpattern_mode = 'first-update') {
  drush_print(dt("Enabling pattern '$id'"));

  $pattern = patterns_get_pattern($id);

  if (!$pattern) {
    drush_set_error(dt("Cannot enable pattern '$id' as it could not be found in the list of registered patterns. Did you import it first?"));
    return FALSE;
  }

  /** defaulting to batch mode & always running subpatterns
   * run-subpatterns choices are:
   'first-update' => t('only if disabled or if updated since last run (recommended)'),
   'always' => t('always'),
   'update' => t('only if updated since last run'),
   'first' => t('only if disabled'),
   'never' => t("don't run sub-patterns at all"),
   @todo make these constants define() statements in patterns.module
   */
  $result = patterns_execute_pattern_drush($pattern, array('run-subpatterns' => $subpattern_mode, 'pid' => $pattern->pid, 'confirm' => 1, 'op' => 'Confirm', 'submit' => 'Confirm'));

  if (!$result) {
    drush_set_error(dt("Enabling pattern '$pattern->pid:$pattern->name' failed"));
    return FALSE;
  }
  drush_print(dt("Success! Enabled pattern '$pattern->pid:$pattern->name'"));
  return TRUE;
}


/**
 * patterns list command callback.
 */
function drush_patterns_list() {
  $patterns = patterns_io_get_patterns();

  $pipe = array();
  $rows[] = array(dt('Id'), dt('Name'), dt('Title'), dt('Status'), dt('Version'));
  foreach ($patterns as $pid => $pattern) {
    $rows[] = array($pattern->pid, $pattern->name, $pattern->title, $pattern->status, $pattern->info['version']);
    $pipe[] = "$pattern->title";
  }

  drush_print_table($rows, TRUE);
  drush_print_pipe(implode(' ', $pipe));
}

/**
 * patterns list command callback.
 * @param mixed int|string $id The pattern id or name of the pattern
 */
function drush_patterns_info($id) {
  $pattern = patterns_get_pattern($id);
  if (!$pattern) {
    drush_set_error(dt("There is no pattern registered with id or name '$id'"));
    return FALSE;
  }
  $info = "";
  $info .= sprintf("  %-18s: %s\n", 'PID', $pattern->pid);
  $info .= sprintf("  %-18s: %s\n", 'Name', $pattern->name);
  $info .= sprintf("  %-18s: %s\n", 'Description', $pattern->description);
  $info .= sprintf("  %-18s: %s\n", 'File', $pattern->file);
  $info .= sprintf("  %-18s: %s\n", 'Status', $pattern->status);
  $info .= sprintf("  %-18s: %s\n", 'Public', $pattern->public);
  $info .= sprintf("  %-18s: %s\n", 'Updated', $pattern->updated);
  $info .= sprintf("  %-18s: %s\n", 'Enabled', $pattern->enabled);

  if (is_array($pattern->pattern['info'])) {
    foreach ($pattern->pattern['info'] as $key => $value) {
      $info .= sprintf("  %-18s: %s\n", ucfirst($key), $value);
    }
  }
  if (is_array($pattern->pattern['modules'])) {
    $info .= sprintf("  %-18s: %s\n", 'Modules', implode(', ', $pattern->pattern['modules']));
  }

  drush_print($info);
}



//NOT CLEANED UP YET

/**
 * patterns list command callback.
 */
function drush_patterns_form_helper($action) {
  if ($action == 'enable') {
    variable_set('patterns_form_helper', TRUE);
  }
  else {
    variable_set('patterns_form_helper', FALSE);
  }
}

/**
 * patterns list command callback.
 */
function drush_patterns_allow_publish($action) {
  if ($action == 'enable') {
    variable_set('patterns_allow_publish', TRUE);
  }
  else {
    variable_set('patterns_allow_publish', FALSE);
  }
}

/**
 * patterns list command callback.
 */
function drush_patterns_paths() {
  $paths = patterns_paths();
  drush_print("Paths to look for patterns:");
  foreach ($paths as $path) {
    drush_print("- $path", 4);
  }

  drush_print_pipe(implode(', ', $paths));
}


/**
 * patterns publish command callback.
 */
function drush_patterns_publish($pid) {
  if (is_numeric($pid)) {
    $result = db_query("UPDATE {patterns} SET public = 1 WHERE pid = %d", $pid);
  }
}

/**
 * patterns unpublish command callback.
 */
function drush_patterns_unpublish($pid) {
  if (is_numeric($pid)) {
    $result = db_query("UPDATE {patterns} SET public = 0 WHERE pid = %d", $pid);
  }
}

/**
 * This is a fork of patterns_execute_patterns_batch(), and will be adapted
 * to use the drush batch api instead
 * @todo (evergreen todo): make sure this is always in sync with
 * patterns_execute_pattern_batch().
 */
function patterns_execute_pattern_drushbatch($pattern, $params = array()) {

  set_time_limit(0);

  if (!is_object($pattern)) {
    $pattern = patterns_get_pattern($pattern);

    if (!$pattern) {
      return FALSE;
    }
  }

  $pattern->subpatterns_run_mode = $params['run-subpatterns'];
  $pattern_details = patterns_parser_get_pattern_details($pattern, TRUE);

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

  $batch = array(
    'title' => t('Processing pattern %pattern', array('%pattern' => $info['title'])),
//    'init_message' => t('Running action @current out of @total', array('@current' => 1, '@total' => count($actions))),
    'progress_message' => t('Running action @current out of @total'),
    'operations' => array(),
    'finished' => 'patterns_batch_finish'
  );

  for ($i = 0; $i < count($actions); $i++) {
    $batch['operations'][] = array('patterns_batch_actions', array($actions[$i], $i, $actions_map));
  }

  $_SESSION['patterns_batch_info'] = $pattern_details['info'];

  batch_set($batch);
  return TRUE;
}

function patterns_execute_pattern_drush($pattern, $params = array()) {

  set_time_limit(0);

  if (!is_object($pattern)) {
    $pattern = patterns_get_pattern($pattern);

    if (!$pattern) {
      return FALSE;
    }
  }

  $pattern->subpatterns_run_mode = $params['run-subpatterns'];
  $pattern_details = patterns_parser_get_pattern_details($pattern, TRUE);

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

  $batch = array(
    'title' => t('Processing pattern %pattern', array('%pattern' => $info['title'])),
//    'init_message' => t('Running action @current out of @total', array('@current' => 1, '@total' => count($actions))),
    'progress_message' => t('Running action @current out of @total'),
    'operations' => array(),
    'finished' => 'patterns_batch_finish_drush'
  );

  for ($i = 0; $i < count($actions); $i++) {
    $batch['operations'][] = array('patterns_batch_actions_drush', array($actions[$i], $i, $actions_map));
  }

  // @todo: this is good enough for testing but it should be
  // implemented in a better way asap
  variable_set('patterns_details', $pattern_details['info']);

  batch_set($batch);
  $batch = &batch_get();
  $batch['progressive'] = FALSE;
  drush_backend_batch_process();

  return TRUE;
}

/**
 * Execute a batch action
 */
function patterns_batch_actions_drush($action, $place, $actions_map, &$context) {

  patterns_io_load_components();

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
  if (!empty($context['results']['abort'])) {
    return;
  }

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
 * Finish a batch operation
 */
function patterns_batch_finish_drush($success, $results, $operations) {
  $info = variable_get('patterns_details', array());

  if (empty($results['abort'])) {
    foreach ($info as $key => $i) {
      drupal_set_message(t('Pattern "@pattern" ran successfully.', array('@pattern' => $i['title'])));
      db_query("UPDATE {patterns} SET status = 1, enabled = '%s' WHERE pid = %d", time(), $key);
    }
  }
  else {
    $pattern = reset($info);
    drupal_set_message(t('Pattern "@pattern" ran with the errors. Check the error messages to get more details.', array('@pattern' => $pattern['title'])));
    drupal_set_message($results['error_message'], 'error');
  }
  variable_del('patterns_details');
  drupal_flush_all_caches();
}

