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
 * Scan the actions_map array and return info about the file the action was coming
 * from with the following info:
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
 * Check if pattern array or content from a file is valid 
 *
 * @param mixed $pattern pattern array or content of a pattern file
 * @return TRUE if valid, FALSE otherwise
 */
function patterns_validate_pattern($pattern) {
  $scan = _patterns_scan_pattern ($pattern);
  return _patterns_validate_patternscan($scan);
}

/**
 * Analyze the result of a call to _patterns_scan_pattern, and check whether 
 * the pattern was valid.
 *
 * @param array $analysis obtained from  _patterns_scan_pattern
 * @return TRUE if valid, FALSE otherwise
 */
function _patterns_analyze_patternscan($patternscan) {
  
  $msgs = array();
    
    if ($patternscan['info'] == 0) {       
        $msgs[] = 'Info section is missing.';
    }
        
    if ($patternscan['info'] > 1) {
        $msgs[] = 'Pattern can contain only one \'info\' section';
    }
        
    if ($patternscan['modules'] > 1) {
        $msgs[] = 'Pattern can contain only one \'modules\' section';
    }
        
    if (count($patternscan['other_sections']) == 0) { 
        $msgs[] = 'Pattern does not contain any action';
    }
    
    if (count($patternscan['invalid_actions']) != 0) { 
        $out = 'Only ' . implode(', ',patterns_actions()) . ' are valid actions. Found: ';
        $out.= implode(', ', $patternscan['invalid_actions']);
        $msgs[] = $out;
    }
        
    if (count($patternscan['empties']) > 0) {
        $msgs[] = 'Pattern contains empty actions: ' . implode(', ', $patternscan['empties']);
    }
  
  return $msgs;
}

/**
 * Analyze the result of a call to _patterns_scan_pattern, and check whether 
 * the pattern was valid.
 *
 * @param array $analysis obtained from  _patterns_scan_pattern
 * @return TRUE if valid, FALSE otherwise
 */
function _patterns_validate_patternscan($patternscan) {
  $analysis = _patterns_analyze_patternscan($patternscan);
  return (empty($analysis)) ? TRUE : FALSE;
}

/**
 * Pattern
 * 
 * // @TODO: Check if we can use patterns_load_yaml instead
 * 
 */
function patterns_parse_yaml($pattern) {
  
 if (!is_array($pattern)) {
    include_once libraries_get_path('spyc') . '/spyc.php';
    $pattern = Spyc::YAMLLoad($pattern);
  }
  return $pattern;
}

/**
 * Scan a pattern and returns a brief summary of its properties
 *
 * @param $pattern
 *   pattern array obtained by parsing pattern file
 * @return
 *   array $result summary of the pattern
 *
 * @todo expand this function to include much more detailed validation
 */
function _patterns_scan_pattern ($pattern) {
  
  $result = array(
                  'info' => 0,
                  'modules' => 0,
                  'empties' => array(),
                  'include' => array(),
                  'invalid_actions' => array(),
                  'other_sections'=> array(),
  );          
  
  $pattern = patterns_parse_yaml($pattern);
  
  if (!empty($pattern)) {
    
    // Patterns must contain:
    // 1 and only section info
    // 0 or 1 section modules
    // 1 or more other sections
    // All the sections must be a non-empty array
    foreach ($pattern as $key => $value) {
      if ($key == 'info') {
        $result['info']++;
      }
      else if ($key == 'modules') {
        $result['modules']++;
      }
      else if ($key == 'include') {
        $result['include']++;
      }
      else {
        $result['other_sections'][$key] = $key; // TODO: add info about the section as value
        
        foreach ($value as $action => $data) {
          
          if (!array_key_exists($action,patterns_actions())) {
            $result['invalid_actions'][$action] = $action; 
          }
        }
      }
      
      if (empty($value)) {
        $result['empties'][] = $key;
      }
    }
  }
  
  return $result;
}



/**
 * Callback of the url patterns/validate. Returns validation info about the 
 * pattern passed in the $_POST array
 * 
 */
function patterns_validate_service () {
    
    $scan = _patterns_scan_pattern($_POST['pattern']);
    
    if (_patterns_validate_patternscan($scan)) {
      $count = count($scan['other_sections']) + $scan['info'] + $scan['modules']; 
      $out = t('This pattern is valid. !count sections found.',array('!count'=>$count));
      //$out.= print_r($scan,true);
    }
    else {
      $errors = implode('<br/> ', _patterns_analyze_patternscan($scan));
      $out = 'This pattern is NOT valid.<br/>' . $errors;
      //$out.= print_r($scan,true); 
    }
 
    print $out;
}

/**
 * Return an array with detailed information about the pattern(s) referenced in
 * the pattern files (included).
 * 
 * @param stdClass $pattern Pattern object as loaded by patterns_get_pattern()
 * @param $recursive if the directive include is valid
 * @param $pids ??
 * 
 * @return array $details array('pid1'=>$details, 'pid2'=>$details...)
 */
function patterns_get_pattern_details($pattern, $recursive = FALSE, &$pids = array()) {

  $scan = _patterns_scan_pattern($pattern->pattern);
  
  $details = array();
  
  if (_patterns_validate_patternscan($scan)) {
    $details['info'] = $pattern->pattern['info'];
    $details['sections'] = array_intersect_key($pattern->pattern,$scan['other_sections']);
    $details['modules'] = ($scan['modules'] > 0 ) ? $pattern->pattern['modules'] : array();
  }
  
  return array($pattern->pid => $details);
  
  ////////////////////////////////////////////////////////////////
  // IMPORTANT
  // The part below is for handling the include tag. Not used now;
  ////////////////////////////////////////////////////////////////
  
  
  // prevent infinite recursion
  // Disabled! Infinite recursion is possible!
  // This allows the same pattern to be re-executed with different parameters
  // TODO: detect recursion, and protect users from it
  // if (in_array($pattern->pid, $pids)) return array();
  $pids[$pattern->pid] = $pattern->pid;


  $patterns[$pattern->pid] = (array)$pattern;
  $patterns[$pattern->pid] = array_merge($patterns[$pattern->pid], $patterns[$pattern->pid]['pattern']['info']);
  unset($patterns[$pattern->pid]['pattern']);

  
  
  if ($recursive) {
    $result = array('modules' => $modules, 'info' => $patterns);

    foreach ($actions as $key => $action) {
      if ($action['tag'] == 'pattern') {
        // determine pattern name
        if (!empty($action['value'])) {
          $name = $action['value'];
        }
        elseif (!empty($action['name'])) {
          $name = $action['name'];
        }
        if (!$p = patterns_get_pattern($name)) {
          // just give a warning and try to continue
          drupal_set_message(t('Action #%key in %file: Pattern %pattern not found.<br/>Pattern execution will try to continue without it.', array('%key' => $key+1, '%file' => $pattern->title, '%pattern' => $name)), 'warning');
          continue;
        }
        
        // Decide if sub-pattern needs to be run based on the mode defined within the pattern or selected in UI at the time of form submission
        // @TODO: UI setting should be able to override a setting defined within the pattern 
        $modes = array('first-update', 'always', 'update', 'first', 'never');
        if (!empty($action['run']) && in_array($action['run'], $modes)) {
          $mode = $action['run'];
        }
        else {
          $mode = $pattern->subpatterns_run_mode;
        }

        switch ($mode) {
          case 'never':
            // don't run sub-pattern 
            drupal_set_message(t('Action #%key in %file: Pattern %pattern not ran because the pattern was set to be skipped.', array('%key' => $key+1, '%file' => $pattern->title, '%pattern' => $name)), 'status'); 
            continue 2;
            break;
          case 'first':
            // Only run on first run
            if ($p->status) {
              drupal_set_message(t('Action #%key in %file: Pattern %pattern not ran because the pattern was set to execute only on the first run.', array('%key' => $key+1, '%file' => $pattern->title, '%pattern' => $name)), 'status');
              continue 2;
            }
            break;
          case 'update':
            // Only run on pattern update
            if ($p->enabled >= $p->updated) {
              drupal_set_message(t('Action #%key in %file: Pattern %pattern not ran because the pattern was set to execute only on pattern update.', array('%key' => $key+1, '%file' => $pattern->title, '%pattern' => $name)), 'status');
              continue 2;
            }
            break;
          case 'first-update':
            // Only run on first run or pattern update
            if ($p->status && $p->enabled >= $p->updated) {
              drupal_set_message(t('Action #%key in %file: Pattern %pattern not ran because the pattern was set to execute only on first run or update.', array('%key' => $key+1, '%file' => $pattern->title, '%pattern' => $name)), 'status');
              continue 2;
            }
            break;
          case 'always':
          default:
            // Run always
            break;
        }

        $a = patterns_get_pattern_details($p, TRUE, $pids);
        if (is_array($a) && empty($a)) {
          // An empty array is returned on infinite recursion detection
          drupal_set_message(t('Action #%key in %file: Infinite recursion detected while attempting to run pattern %pattern.<br/>Pattern execution will try to continue without it.', array('%key' => $key+1, '%file' => $pattern->title, '%pattern' => $name)), 'warning');
          continue;
        }
        // we replace for tokens in the generated pattern
        // this is just a proof of concept, so far
        if (!empty($action['parameters'])) {
          $tokens = array_keys($action['parameters']);
          $values = array_values($action['parameters']);
          // give tokens their delimiters
          foreach ($tokens as &$token) {
            $token = "__" . $token . "__";
          }
          $a = patterns_array_map('str_replace', $a, array($tokens, $values));
        }
        // array_merge doesn't preserve numeric array keys
        // so we handle 'info' separately
        $info = $result['info'];
        $result = array_merge_recursive($result, $a);
        $result['info'] = $info + $a['info'];
      }
      else {
        $result['actions'][] = $action;
        $result['actions_map'][] = array(
          'pid'   => $pattern->pid,
          'index' => $key,
        );
      }
    }
    $result['modules'] = array_merge(array_unique($result['modules']));
    // Remove pid from recursion stack
    //unset($pids[$pattern->pid]);
    return $result;
  }
  // Remove pid from recursion stack
  //unset($pids[$pattern->pid]);
  return array('actions' => $actions, 'modules' => $modules, 'info' => $patterns);
}