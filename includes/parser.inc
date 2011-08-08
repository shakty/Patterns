<?php


/**
 * Find parts of an array based on
 * a semi-compatible xpath syntax.
 *
 * Returns an array of constructs that includes the
 * references 'item' and 'parent' from the matching values
 * in the $data object along with extra keys 'key' for the key
 * of the current match and 'trace' for a full list of keys
 * till the root of the $data object
 *
 * Loosely based off of Cake function Set::extract
 *
 * @Note: Ensure this always only returns matches from a single level
 * in the array. Changes made to matches in different levels can possibly
 * mess up the above-level matches. To this end, you cannot change the keys
 * of a current match because the other matches cannot change their keys as well
 * to sync with the new parent array.
 * @TODO: Think of a solution for the above note (BROKEN RIGHT NOW)
 */
function patterns_array_fetch($path, &$data, $context = NULL) {
  if (!$context && (empty($data) || !is_array($data))) {
    return array();
  }
  if ($path === '/') {
    return $data;
  }

  // Construct our contexts object that allows us to traverse the array
  if (!$context) {
    $context = _patterns_array_context($data);
  }

  // Make our context actually a list of contexts
  $context = array($context);

  // Create a list of tokens based on the supplied path
  $tokens = array_slice(preg_split('/(?<!=)\/(?![a-z]*\])/', $path), 1);

  while (!empty($tokens)) {
    $token = array_shift($tokens);

    // TODO Implement better conditionals for each token
    // Currently only supports element=value conditions
    $conditions = array();
    if (preg_match('/(=)(.*)/', $token, $m)) {
      $conditions[$m[1]] = $m[2];
      $token = substr($token, 0, strpos($token, $m[1]));
    }

    $matches = array();
    foreach ($context as &$piece) {
      if ($token === '..') {
        $matches[] = &$piece['parent'];
        continue;
      }
      $match = FALSE;
      if (is_array($piece['item']) && ($token == '*' || array_key_exists($token, $piece['item']))) {
        $i = 0;
        while (isset($piece[$i])) {
          if ($piece[$i]['key'] === $token) {
            $matches[] = &$piece[$i];
            break;
          }
          elseif ($token === '*') {
            $matches[] = &$piece[$i];
          }
          $i++;
        }
      }
      elseif ($token === '.') {
        $matches[] = &$piece;
      }
    }

    // Filter matches from the matches list based on our conditions
    foreach ($conditions as $operator => $value) {
      _patterns_array_filter($matches, $operator, $value);
    }

    // Update the context area to the next set of matches to dig into
    $context = $matches;
  }

  // Return the list of matches containing references to their respective data objects
  return $matches;
}


/**
 * Recurse through the values of a parsed xml file to create a
 * multi-dimensional representation of the data.
 */
function _patterns_parse_tag($data, &$index = 0) {
  $pattern = array();

  while (isset($data[$index]) && $current = $data[$index]) {
    $type = $current['type'];

    if (!empty($current['attributes'])) {
      foreach ((array)$current['attributes'] as $key => $value) {
        $current[$key] = $value;
      }
    }

    unset($current['type'], $current['level'], $current['attributes']);

    if (isset($current['value']) && !trim($current['value']) && $current['value'] != "0") {
      unset($current['value']);
    }

    switch ($type) {
      case 'open':
        $index++;
        $current += _patterns_parse_tag($data, $index);
        $pattern[] = $current;
        break;
      case 'close':
        $index++;
        return $pattern;
        break;
      case 'complete':
        // In order to support more complex/non-standard features we can use serialized data
        if (!empty($current['attributes']['serialized'])) {
          $value = unserialize($current['value']);

          if (isset($value)) {
            $current['value'] = $value;
          }
        }

        // If no value was specified, make sure an empty value is there
        if (!isset($current['value'])) {
          $current['value'] = '';
        }

        $pattern[] = $current;
        break;
    }

    $index++;
  }

  return $pattern;
}


/**
 * Create a concise description of the Action.
 * 
 * Returns an array containing the following info:
 * 
 * - index of the action within the batch_set
 * - title of the pattern
 * - file of the pattern
 * 
 * @param mixed $key The key of the action in the actions_map array
 * @param array $actions_map array containing information about of all the
 * actions of the batch_set
 * @return array $result concise description of the action
 */
function patterns_locate_action($key, $actions_map) {
  $result['key'] = $actions_map['map'][$key]['index'];
  $result['title'] = $actions_map['patterns'][$actions_map['map'][$key]['pid']]['title'];
  $result['file'] = $actions_map['patterns'][$actions_map['map'][$key]['pid']]['file'];
  return $result;
}


/**
 * Check if pattern array contains only allowed keys
 *
 * @param $pattern
 *   pattern array obtained by parsing pattern file
 * @return
 *   TRUE when only allowed array keys are found, FALSE otherwise
 *
 * @todo expand this function to include much more detailed validation
 */
function patterns_validate_pattern($pattern) {
  if (empty($pattern)) {
    return FALSE;
  }

  $allowed_keys = array('info', 'modules', 'actions');
  $diff = array_diff(array_keys($pattern), $allowed_keys);
  return empty($diff) ? TRUE : FALSE;
}

