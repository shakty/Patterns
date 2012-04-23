<?php

/**
 * @file
 * Code related to send pattern to server / receive pattern as server.
 */

/**
 * Implements hook_drupaltodrupal_secure_rpc().
 */
function patterns_d2d_drupaltodrupal_secure_rpc() {
  $methods = array();
  $methods['patterns_d2d_push_patterns'] = array(
    'arguments' => array('patterns' => 'is_string'),
    'callback' => 'patterns_d2d_push_patterns_server',
    'description' => 'publish public patterns to remote server',
  );
  return $methods;
}
function patterns_d2d_push_patterns_server($arguments, $rpc_info) {
  $imploded_patterns = $arguments['patterns'];
  $exploded_patterns = drupaltodrupal_explode($imploded_patterns);
  if ($exploded_patterns === FALSE) {
    throw new DrupalToDrupalRemoteException('internal error while processing patterns');
  }
  $n_patterns = 0;
  foreach ($exploded_patterns as $imploded_pattern) {
    $exploded_pattern = drupaltodrupal_explode($imploded_pattern);
    if ($exploded_pattern !== FALSE) {
      $check = array(
        'name' => 'is_string',
        'format' => 'is_string',
        'content' => 'is_string',
      );
      if (drupaltodrupal_check_array($exploded_pattern, $check)) {
        $name = $rpc_info['id'] . '_' . $exploded_pattern['name'];
        if (patterns_io_save_pattern($exploded_pattern['content'], $name, $exploded_pattern['format'])) {
          patterns_db_publish_pattern($name);
          $n_patterns++;
        }
      }
    }
  }
  return $n_patterns . ' pattern(s) received';
}
function patterns_d2d_push_patterns_client($patterns) {
  $url = variable_get('patterns_d2d_server', PATTERNS_D2D_SERVER);
  $friend = patterns_d2d_get_friend($url);
  if ($friend === FALSE) {
    drupal_set_message(t('Patterns server not listed as friend instance.'), 'error');
    return;
  }
  $upload_patterns = array();
  foreach ($patterns as $pattern) {
    //drupal_set_message(var_export($pattern, TRUE));
    $name = $pattern->name;
    $format = $pattern->format;
    /*
    $raw_pattern = $pattern->pattern;
    $dumped_pattern = patterns_parser_dump($raw_pattern, $format);
    */
    $content = file_get_contents($pattern->file);
    if ($content === FALSE) {
      drupal_set_message(t('Pattern') . ' ' . check_plain($name) . ' ' . t('could not be processed'), 'error'); // TODO: proper t()
      continue;
    }
    $imploded_pattern = drupaltodrupal_implode(
      array(
        'name' => $name,
        'format' => $format,
        'content' => $content,
      )
    );
    if ($imploded_pattern !== FALSE) {
      $upload_patterns[] = $imploded_pattern;
    }
  }
  $error_string = '';
  $imploded_patterns = drupaltodrupal_implode($upload_patterns);
  if ($imploded_patterns === FALSE) {
    $res = FALSE;
    $error_string = t('Internal error while collecting patterns to be sent.');
  }
  else {
    $res = drupaltodrupal_call_secure_rpc($friend, 'patterns_d2d_push_patterns', array('patterns' => $imploded_patterns), $error_string);
  }
  if ($res === FALSE) {
    drupal_set_message(check_plain($url) . ' - ' . check_plain($error_string), 'error');
  }
  else {
    drupal_set_message('Upload to server successful.<br/>' . check_plain($url) . ' - ' . check_plain($res)); // TODO: proper t()
  }
}
