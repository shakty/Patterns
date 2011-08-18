<?php 

function patterns_set_error_handler() {
  // set custom error handler
  set_error_handler('patterns_error_handler');
  // trigger dummy error
  // this will be used as a refrence to determine if any real error
  // occured during the pattern execution
  @trigger_error('patterns_error');
}

function patterns_restore_error_handler() {
  restore_error_handler();
}

/**
 * Custom error handler used only during patterns execution
 * in order to catch and properly handle PHP errors.
 * Based on drupal_error_handler().
 */
function patterns_error_handler($errno, $message, $filename, $line, $context) {

  // @TODO  E_DEPRECATED does not work in PHP < 5.3
  if ($errno & (E_ALL ^ E_NOTICE)) { // ^ E_DEPRECATED)) {

    $types = array(
      1 =>array('Error', WATCHDOG_ERROR),
      2 => array('Warning', WATCHDOG_WARNING),
      4 => array('Parse error', WATCHDOG_ERROR), 
      8 => array('Notice', WATCHDOG_NOTICE),
      16 => array('Core error', WATCHDOG_ERROR),
      32 => array('Core warning', WATCHDOG_WARNING),
      64 => array('Compile error', WATCHDOG_ERROR),
      128 =>  array('Compile warning', WATCHDOG_WARNING),
      256 => array('User error', WATCHDOG_ERROR),
      512 => array('User warning', WATCHDOG_WARNING),
      1024 => array('User notice', WATCHDOG_NOTICE),
      2048 => array('Strict warning', WATCHDOG_DEBUG),
      4096 => array('Recoverable fatal error', WATCHDOG_ERROR),
     );

    // For database errors, we want the line number/file name of the place that
    // the query was originally called, not _db_query().
    if (isset($context['DB_ERROR'])) {
      $backtrace = array_reverse(debug_backtrace());

      // List of functions where SQL queries can originate.
      $query_functions = array('db_query', 'pager_query', 'db_query_range', 'db_query_temporary', 'update_sql');

      // Determine where query function was called, and adjust line/file
      // accordingly.
      foreach ($backtrace as $index => $function) {
        if (in_array($function['function'], $query_functions)) {
          $line = $backtrace[$index]['line'];
          $filename = $backtrace[$index]['file'];
          break;
        }
      }
    }

    // 'patterns_error' is not a real error and should be skipped
    if ($message != 'patterns_error') {
      watchdog('php', '%message in %file on line %line.', array('%error' => $types[$errno][0], '%message' => $message, '%file' => $filename, '%line' => $line), $types[$errno][1]);
      if($types[$errno][1]==WATCHDOG_ERROR)
        patterns_error_set_last(array('message' => $message, 'type' => $errno, 'file' => $filename, 'line' => $line));
    }
  }
  return TRUE;
}

function patterns_error_set_last($error = NULL) {
  static $last_error = array();

  if (isset($error)) {
    $last_error = $error;
  }
  return $last_error;
}

/**
 * Check and report PHP errors during patterns execution
 *
 * @param $op
 *   operation within hook_patterns() during which error occured
 * @param $key
 *   number of the action currently proccessed
 * @param $description
 *   description of the current action
 * @param $pattern_title
 *   title of the pattern currently proccessed
 * @param $pattern_file
 *   path to pattern file currently proccessed
 * @return
 *   error message if new error encountered
 *   FALSE if there are no new errors
 *
 */
function patterns_error_get_last($op, $key, $description, $pattern_title, $pattern_file) {
  $error = patterns_error_set_last();

  if (!empty($error) && $error['message'] != 'patterns_error') {
   // $types = array(1 => 'error', 2 => 'warning', 4 => 'parse error', 8 => 'notice', 16 => 'core error', 32 => 'core warning', 64 => 'compile error', 128 => 'compile warning', 256 => 'user error', 512 => 'user warning', 1024 => 'user notice', 2048 => 'strict warning', 4096 => 'recoverable fatal error');
       $types = array(
      1 =>array('Error', WATCHDOG_ERROR),
      2 => array('Warning', WATCHDOG_WARNING),
      4 => array('Parse error', WATCHDOG_ERROR), 
      8 => array('Notice', WATCHDOG_NOTICE),
      16 => array('Core error', WATCHDOG_ERROR),
      32 => array('Core warning', WATCHDOG_WARNING),
      64 => array('Compile error', WATCHDOG_ERROR),
      128 =>  array('Compile warning', WATCHDOG_WARNING),
      256 => array('User error', WATCHDOG_ERROR),
      512 => array('User warning', WATCHDOG_WARNING),
      1024 => array('User notice', WATCHDOG_NOTICE),
      2048 => array('Strict warning', WATCHDOG_DEBUG),
      4096 => array('Recoverable fatal error', WATCHDOG_ERROR),
     );
    
    $php_error_message = $types[$error['type']][0] . ': ' . $error['message'] . ' in ' . $error['file'] . ' on line ' . $error['line'] . '.';
    $message = t('Pattern %title (%pattern_file)<br/>Action #%key: %description (op "%op")<br/>PHP error occured:<br/>%error', array('%key' => $key, '%title' => $pattern_title, '%op' => $op, '%description' => $description, '%error' => $php_error_message, '%pattern_file' => $pattern_file));
    return $message;
  }
  return FALSE;
}