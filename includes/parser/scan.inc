<?php

/**
 * @file
 *
 * Helping functions for the parser.
 *
 * Functions related to parsing the abstract array structure
 *
 */


/**
 * Scans a pattern and returns a brief summary of its properties.
 *
 * @param array $pattern Pattern array obtained by parsing pattern file.
 * @param bool $include (optional) If TRUE, actions can be found outside
 *   of sections. Default FALSE.
 *
 * @return array $result Summary of the pattern.
 *
 * @TODO Expand this function to include much more detailed validation.
 * @TODO Rewrite in an object oriented fashion
 *
 * @see _patterns_scan_action()
 * @see _patterns_scan_analyze_patternscan()
 *
 */
function patterns_scan_pattern($pattern, $include = FALSE) {
  $generic_error = t('pattern not loaded correctly');
  $result = array(
                  'info' => 0,
                  'modules' => 0,
                  'empties' => array(),
                  'invalid_actions' => array(),
                  'extra_actions' => array(),
                  'missing_tag' => array(),
                  'other_sections' => array(),
                  'contain_includes' => FALSE,
                  'include_scans' => array(),
                  'generic_errors' => array(),
  );

  if (empty($pattern)) {
    $result['empties'][] = $generic_error;
    return $result;
  }

  // Patterns is valid if contains:

  //  - exactly 1 topmost section 'info'
  //  - at most 1 topmost section 'modules'
  //  - at least 1 more other sections
  //
  // All the sections must be a non-empty array.
  // If a section is called 'include;
  // it means that we are including a new pattern
  // and we apply patterns_scan_pattern recursively.

  foreach ($pattern as $section_name => $section) {

    // INFO
    if ($section_name === 'info') {
      $result['info']++;
      continue;
    }

    // MODULES
    if ($section_name === 'modules') {
      $result['modules']++;
      continue;
    }

    // SECTIONS or ACTIONS (if it is an include)
    if (!_patterns_scan_is_section($section)) {
      if (!$include) {
        $result['generic_errors'][] = $section_name ;
        continue;
      }

      $result = _patterns_scan_action($section, $result);
      continue;
    }

    // SECTIONS: invalid
    // included patterns can be just the name or the id
    // of the pattern
    if (!is_array($section) && !$include) {
      // If the YAML is not loaded correctly you get 0.
      $result['empties'][] = ($section_name === 0) ? $generic_error
                                                   : $section_name;
      continue;
    }

    // SECTIONS: valid
    //$resultactions = array();
    //$newactions = array();

    // Collect info specific to each section
    $section_info = array();
    $section_info[PATTERNS_CREATE] = 0;
    $section_info[PATTERNS_MODIFY] = 0;
    $section_info[PATTERNS_DELETE] = 0;
    $section_info[PATTERNS_INCLUDE] = 0;

    foreach ($section as $key => &$action) {
      $action_name = key($action);
      if (isset($section_info[$action_name])) {
        // Incremente the count of valid actions
        $section_info[$action_name] = $section_info[$action_name]+1;
      }
      $result = _patterns_scan_action($action, $result);
    }

    $section_info_str = '(' . PATTERNS_CREATE . ':' . $section_info[PATTERNS_CREATE] . ', ' .
                              PATTERNS_MODIFY . ':' . $section_info[PATTERNS_MODIFY] . ', ' .
                              PATTERNS_DELETE . ':' . $section_info[PATTERNS_DELETE] . ', ' .
                              PATTERNS_INCLUDE . ':' . $section_info[PATTERNS_INCLUDE] . ')';

    $result['other_sections'][$section_name] =  $section_info_str;

  }

  return $result;
}

/**
 * Analyze the result of a call to patterns_scan_pattern, and check whether
 * the pattern was valid.
 *
 * @param array $analysis Obtained from  patterns_scan_pattern.
 * @param bool $include (optional) If TRUE, it means that the pattern
 *   to scan is an include, and a laxer scan is performed. E.g.
 *   Info section can be missing, actions can be outside of a section.
 *   Default FALSE.
 *
 * @return TRUE if valid, FALSE otherwise
 */
function _patterns_scan_analyze_patternscan($patternscan, $include = FALSE) {
  $msgs = array();

  if ($patternscan['info'] == 0 && !$include) {
    $msgs[] = t('The info section is missing.');
  }

  if ($patternscan['info'] > 1) {
    $msgs[] = t('Pattern can contain only one \'info\' section.');
  }

  if ($patternscan['modules'] > 1) {
    $msgs[] = t('Pattern can contain only one \'modules\' section.');
  }

  if (count($patternscan['other_sections']) == 0 && !$include) {
    $msgs[] = t('Pattern does not contain any actions.');
  }

  if (count($patternscan['generic_errors']) != 0) {
    $msgs[] = t('Generic errors in the patterns were found. Probably a tag was misplaced. Please verify: %found',
            array('%found' => implode(', ', $patternscan['generic_errors'])));
  }

  if (count($patternscan['invalid_actions']) != 0) {
    $invalidactions = array();
    foreach ($patternscan['invalid_actions'] as $key => $value) {
      $invalidactions[] = $value['key'];
    }
    $msgs[] = t('Only %actions are valid actions. Found: %found.',
            array('%actions' => implode(', ', patterns_actions()),
                  '%found' => implode(', ', $invalidactions)));
  }

  if (count($patternscan['extra_actions']) != 0) {
    $extraactions = array();
    foreach ($patternscan['extra_actions'] as $key => $value) {
      $extraactions[] = $value['key'];
    }
    $msgs[] = t('Extra actions have been found on one level: %found.',
            array('%found' => implode(', ', $extraactions)));
  }

  // TODO: This is not yet working properly. Check when it is applicable!
  if (count($patternscan['missing_tag']) != 0) {
    foreach ($patternscan['missing_tag'] as $key => $value) {
      $msgs[] = t('A mandatory \'tag\' was missing for action %action.', array('%action' => $value['key']));
    }
  }

  if (count($patternscan['empties']) > 0) {
    $msgs[] = t('Pattern contains empty sections or actions:') . ' ' . implode(', ', $patternscan['empties']);
  }

  if (count($patternscan['include_scans']) > 0) {
    foreach ($patternscan['include_scans'] as $i) {
      //$msgs[] = print_r($patternscan['includes'], true);
      $msgs = array_merge($msgs, _patterns_scan_analyze_patternscan($i, TRUE));
    }
  }

  return $msgs;
}

/**
 * Analyze the result of a call to patterns_scan_pattern, and check whether
 * the pattern was valid.
 *
 * @param array $analysis Obtained from  patterns_scan_pattern.
 *
 * @return bool TRUE if valid, FALSE otherwise.
 */
function _patterns_scan_validate_patternscan($patternscan, $display_errors = FALSE) {
  $analysis = _patterns_scan_analyze_patternscan($patternscan);
  if (empty($analysis)) {
    return TRUE;
  }
  if ($display_errors) {
    drupal_set_message(t('Error(s) while processing pattern:') . '<ul><li>' . implode('</li><li>', $analysis) . '</li></ul>', 'error');
  }
  return FALSE;
}

/**
 * Determines whether an array is actually a section.
 *
 * A section is an array, whose first element is a valid
 * action.
 *
 * @param array $section The array to check
 *
 * @return Boolean TRUE if the array is a section
 *
 * @see _patterns_scan_is_action()
 */
function _patterns_scan_is_section($section = NULL) {
  if (is_null($section)) return FALSE;
  if (!is_array($section)) return FALSE;
  $action = array_pop($section);
  return _patterns_scan_is_action($action);
}

/**
 * Determines whether an array is actually an action.
 *
 * An action is an array, whose key name is a valid
 * action name.
 *
 * @param array $action The array to check
 *
 * @return Boolean TRUE if the array is an action
 *
 * @see _patterns_scan_is_section()
 */
function _patterns_scan_is_action($action = NULL) {
  if (is_null($action)) return FALSE;
  if (!is_array($action)) return FALSE;
  return patterns_parser_is_valid_action_name(key($action));
}

/**
 * Helper function for scanning patterns.
 *
 * Scans an action and add the result of the scan to the $result
 * array.
 *
 * @param mixed $key The type of action (e.g. create, modify, delete,
 *   include).
 * @param array $action The action to scan
 * @param array $result (optional) The array in which inserting
 *   the scan results.
 *
 * @return array $result The results of the scan.
 *
 * @see patterns_scan_pattern()
 * @see _patterns_scan_analyze_patternscan()
 *
 */
function _patterns_scan_action($action, $result = array()) {

  $key = key($action);
  $action = current($action);
  $valid_actions = patterns_actions();
  if (!array_key_exists($key, $valid_actions)) {
    $result['invalid_actions'][] = array('actionid' => $action, 'key' => $key, 'value' => $action);
    return $result;
  }

  if (empty($action)) {
    $result['empties'][] = $key;
    return $result;
  }

  if (!is_array($action)) {
    $result['generic_errors'][] = $key . ': ' . $action;
    return $result;
  }

  // Make sure there is only one valid action in each array element.
  // NOTE: Having additional valid actions will work, but is discouraged,
  // and therefore undocumented.
  $found = FALSE;

  // Report additional valid actions.
  if ($found) {
    $result['extra_actions'][] = array('actionid' => $action, 'key' => $key, 'value' => $action);
  }

  // Do action specific checkings
  ////////////////////////////////

  if ($key === PATTERNS_INCLUDE) {
    $result['contain_includes'] = TRUE;

    if (!isset($action['pattern'])) {
      $result['missing_tag'][] = array('actionid' => $action, 'key' => $key, 'value' => $action);
      return $result;
    }

    // only if the pattern is hard-coded, then scan it
    if (is_array($action['pattern'])) {
      $result['include_scans'][] = patterns_scan_pattern($action['pattern'], TRUE);
    }
    return $result;
  }

  // Check if 'tag' is present.
  if (!isset($action['tag']) || !is_string($action['tag'])) {
    $result['missing_tag'][] = array('actionid' => $action, 'key' => $key, 'value' => $action);
  }

  //else {
  //  $newactions[] = array('action' => $key, 'data' => $action);
  //}
  $found = TRUE;

  return $result;
}
