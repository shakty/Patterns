<?php

/**
 * @file
 * Functions related to parsing pattern files.
 */

/**
 * Scan the actions_map array and return info about the file the action was coming
 * from with the following info:
 *
 * - index of the action within the batch_set
 * - title of the pattern
 * - file of the pattern
 *
 * @param mixed $key The key of the action in the actions_map array.
 * @param array $actions_map
 *   Array containing information about of all the actions of the batch_set.
 *
 * @return array $result Concise description of the action.
 */
function patterns_locate_action($key, $actions_map) {
  $result['key'] = $actions_map['map'][$key]['index'];
  $result['title'] = $actions_map['patterns'][$actions_map['map'][$key]['pid']]['title'];
  $result['file'] = $actions_map['patterns'][$actions_map['map'][$key]['pid']]['file'];
  return $result;
}


/**
 * Tries to parse a pattern according to the specified format.
 * If succesfull returns the array representation of the pattern, if not
 * returns FALSE;
 * 
 * @param mixed $pattern A string representation of the pattern, or a pattern 
 *  array. In the latter case, the array is returned as it is, and the @param 
 *  $format is ignored.
 * @param mixed $format The format against which parse the pattern string.
 * 
 */
function patterns_parser_parse($pattern, $format = PATTERNS_FORMAT_UNKNOWN) {
    if (empty($pattern)) return FALSE;
    if (is_array($pattern)) return $pattern;
    
    if ($format == PATTERNS_FORMAT_XML) {
      return patterns_xmlparser_parse($pattern);
    }
    
    if ($format == PATTERNS_FORMAT_YAML) {
      return patterns_yamlparser_parse($pattern);
    }
    
    // Cannot be PATTERNS_FORMAT_PHP
    // otherwise should be an array
    
    return FALSE;
}

/**
 * Tries to parse a pattern according to the specified format.
 * If succesfull returns the array representation of the pattern, if not
 * returns FALSE;
 * 
 * @param mixed $pattern A string representation of the pattern, or a pattern 
 *  array. In the latter case, the array is returned as it is, and the @param 
 *  $format is ignored.
 * @param mixed $format The format against which parse the pattern string.
 * 
 */
function patterns_parser_dump($pattern, $format = PATTERNS_FORMAT_UNKNOWN) {
    if (empty($pattern)) return FALSE;
    if (is_string($pattern)) return $pattern;
    if (!is_array($pattern)) return FALSE;
    
    if ($format == PATTERNS_FORMAT_XML) {
      return patterns_xmlparser_dump($pattern);
    }
    
    if ($format == PATTERNS_FORMAT_YAML) {
      return patterns_yamlparser_dump($pattern);
    }
    
		if ($format == PATTERNS_FORMAT_PHP) {
      return patterns_yamlparser_dump($pattern);
    }
    // Cannot be PATTERNS_FORMAT_PHP
    // otherwise should be an array
    
    return FALSE;
}

/**
 * Check if pattern array or content from a file is valid.
 *
 * @param mixed $pattern Pattern array or content of a pattern file.
 * @param mixed $format The format of the file to validate against.
 * 
 * @return bool TRUE if valid, FALSE otherwise.
 */
function patterns_validate_pattern($pattern, $format = PATTERNS_FORMAT_UNKNOWN) {
  $pattern = patterns_parser_parse($pattern, $format);
  if (!$pattern) return FALSE;
  
  $scan = _patterns_scan_pattern($pattern);
  return _patterns_validate_patternscan($scan);
}

/**
 * Analyze the result of a call to _patterns_scan_pattern, and check whether
 * the pattern was valid.
 *
 * @param array $analysis Obtained from  _patterns_scan_pattern.
 *
 * @return TRUE if valid, FALSE otherwise
 */
function _patterns_analyze_patternscan($patternscan) {
  $msgs = array();

  if ($patternscan['info'] == 0) {
    $msgs[] = t('The info section is missing.');
  }

  if ($patternscan['info'] > 1) {
    $msgs[] = t('Pattern can contain only one \'info\' section.');
  }

  if ($patternscan['modules'] > 1) {
    $msgs[] = t('Pattern can contain only one \'modules\' section.');
  }

  if (count($patternscan['other_sections']) == 0) {
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
      $msgs = array_merge($msgs, _patterns_analyze_patternscan($i));
    }
  }

  return $msgs;
}

/**
 * Analyze the result of a call to _patterns_scan_pattern, and check whether
 * the pattern was valid.
 *
 * @param array $analysis Obtained from  _patterns_scan_pattern.
 *
 * @return bool TRUE if valid, FALSE otherwise.
 */
function _patterns_validate_patternscan($patternscan, $display_errors = FALSE) {
  $analysis = _patterns_analyze_patternscan($patternscan);
  if (empty($analysis)) {
    return TRUE;
  }
  if ($display_errors) {
    drupal_set_message(t('Error(s) while processing pattern:') . '<ul><li>' . implode('</li><li>', $analysis) . '</li></ul>', 'error');
  }
  return FALSE;
}

/**
 * Scans a pattern and returns a brief summary of its properties.
 *
 * @param $pattern
 *   pattern array Obtained by parsing pattern file.
 * @return
 *   array $result Summary of the pattern.
 *
 * @TODO Expand this function to include much more detailed validation.
 * @TODO Rewrite in an object oriented fashion
 *
 */
function _patterns_scan_pattern($pattern) {
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

  // TODO: it will take only arrays at the beginning
  if (!is_array($pattern)) {
    $pattern = patterns_yamlparser_parse($pattern); // TODO: other formats as well.
  }

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
  // and we apply _patterns_scan_pattern recursively.

  $valid_actions = patterns_actions();
  foreach ($pattern as $key => $value) {

    // INFO
    if ($key === 'info') {
      $result['info']++;
      continue;
    }

    // MODULES
    if ($key === 'modules') {
      $result['modules']++;
      continue;
    }

    // SECTIONS: invalid
    if (!is_array($value)) {
      // If the YAML is not loaded correctly you get 0.
      $result['empties'][] = ($key === 0) ? $generic_error : $key;
      continue;
    }

    // SECTIONS: valid
    $resultactions = array();
    $newactions = array();

    // Collect info specific to each section
    $section_info = array();
    $section_info[PATTERNS_CREATE] = 0;
    $section_info[PATTERNS_MODIFY] = 0;
    $section_info[PATTERNS_DELETE] = 0;
    $section_info[PATTERNS_INCLUDE] = 0;

    foreach ($value as $action => &$data) {

      if (empty($data)) {
        $result['empties'][] = $action;
        continue;
      }

      if (!is_array($data)) {
        $result['generic_errors'][] = $action . ': ' . $data; 
        continue;
      }

      // Make sure there is only one valid action in each array element.
      // NOTE: Having additional valid actions will work, but is discouraged,
      // and therefore undocumented.
      $found = FALSE;

      foreach ($data as $akey => $avalue) {

        if (!array_key_exists($akey, $valid_actions)) {
          $result['invalid_actions'][] = array('actionid' => $action, 'key' => $akey, 'value' => $avalue);
          continue;
        }


        // Update counter of the action;
        $section_info[$akey] = $section_info[$akey]+1;

        // Report additional valid actions.
        if ($found) {
          $result['extra_actions'][] = array('actionid' => $action, 'key' => $akey, 'value' => $avalue);
        }

        // Do action specific checkings
        ////////////////////////////////

        if ($akey === PATTERNS_INCLUDE) {
          $result['contain_includes'] = TRUE;

          if (!isset($avalue['pattern'])) {
            $result['missing_tag'][] = array('actionid' => $action, 'key' => $akey, 'value' => $avalue);
            continue;
          }

          // only if the pattern is hard-coded, then scan it
          if (is_array($avalue['pattern'])) {
            $result['include_scans'][] = _patterns_scan_pattern($avalue['pattern']);
          }
          continue;
        }


        // Check if 'tag' is present.
        if (!isset($avalue['tag']) || !is_string($avalue['tag'])) {
          $result['missing_tag'][] = array('actionid' => $action, 'key' => $akey, 'value' => $avalue);
        }
        else {
          $newactions[] = array('action' => $akey, 'data' => $avalue);
        }
        $found = TRUE;


      }
    }
    $value = $newactions;

    $result['other_sections'][$key] =  '(' . PATTERNS_CREATE . ':' . $section_info[PATTERNS_CREATE] . ', ' .
                                       PATTERNS_MODIFY . ':' . $section_info[PATTERNS_MODIFY] . ', ' .
                                       PATTERNS_DELETE . ':' . $section_info[PATTERNS_DELETE] . ', ' .
                                       PATTERNS_INCLUDE . ':' . $section_info[PATTERNS_INCLUDE] . ')';



  }

  return $result;
}



/**
 * Callback of the url patterns/validate.
 *
 * Returns validation info about the pattern passed in the $_POST array.
 */
function patterns_validate_service() {
  
  if (!isset($_POST['pattern'])) {
    print t('No pattern found. Communication error.');
    exit;
  }
  
  if (!isset($_POST['format']) || $_POST['format'] == PATTERNS_FORMAT_UNKNOWN) {
    print t('Cannot validate unknown pattern format');
    exit;
  }
  
  $pattern = patterns_parser_parse($_POST['pattern'], $_POST['format']);
  
  $scan = _patterns_scan_pattern($pattern);

	if (_patterns_validate_patternscan($scan)) {
    $count = count($scan['other_sections']) + $scan['info'] + $scan['modules'];
    $out = t('This pattern is valid. !count sections found ->', array('!count' => $count)) . ' ';

    $out .= 'info ';
    $out .= ($scan['modules']) ? 'modules ' : '' ;

    foreach ($scan['other_sections'] as $key => $value) {
      $out .= $key . ' ' . $value;
    }
  }
  else {
    $errors = implode('<br/> ', _patterns_analyze_patternscan($scan));
    $out = t('This pattern is NOT valid.') . '<br/>' . $errors;
  }

  print $out;
}

/**
 * Returns an array with detailed information about the pattern(s) referenced in
 * the pattern files (included).
 *
 * @param stdClass $pattern Pattern object as loaded by patterns_get_pattern().
 * @param bool $recursive If the directive include is valid.
 * @param array $pids ??
 *
 * @return array $details
 *   array('pid1' => $details, 'pid2' => $details, ...),
 *   FALSE if there were errors (in which case the errors will be displayed).
 */
function patterns_parser_get_pattern_details($pattern, $recursive = FALSE, &$pids = array()) {
  $scan = _patterns_scan_pattern($pattern->pattern);
  // This will display errors if there are any.
  if (!_patterns_validate_patternscan($scan, TRUE)) return FALSE;

  $details = array();

  $details['info'] = $pattern->pattern['info'];
  $details['sections'] = array_intersect_key($pattern->pattern, $scan['other_sections']);
  $details['modules'] = ($scan['modules'] > 0 ) ? $pattern->pattern['modules'] : array();
  //if (!$scan['contain_includes'])
    return array($pattern->pid => $details); // TODO: only one pattern
  //}



  ////////////////////////////////////////////////////////////////
  // IMPORTANT
  // The part below is for handling the include tag. Not used now
  // and probably deprecated.
  ////////////////////////////////////////////////////////////////


  // prevent infinite recursion
  // Disabled! Infinite recursion is possible!
  // This allows the same pattern to be re-executed with different parameters
  // TODO: detect recursion, and protect users from it
  // if (in_array($pattern->pid, $pids)) return array();
  $pids[$pattern->pid] = $pattern->pid;


  $patterns[$pattern->pid] = (array) $pattern;
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
          drupal_set_message(t('Action #%key in %file: Pattern %pattern not found.<br/>Pattern execution will try to continue without it.', array('%key' => $key + 1, '%file' => $pattern->title, '%pattern' => $name)), 'warning');
          continue;
        }


        $a = patterns_parser_get_pattern_details($p, TRUE, $pids);
        if ($a === FALSE) { // TODO: error handling
        }
        if (is_array($a) && empty($a)) {
          // An empty array is returned on infinite recursion detection
          drupal_set_message(t('Action #%key in %file: Infinite recursion detected while attempting to run pattern %pattern.<br/>Pattern execution will try to continue without it.', array('%key' => $key + 1, '%file' => $pattern->title, '%pattern' => $name)), 'warning');
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
  // Remove pid from recursion stack.
  // unset($pids[$pattern->pid]);
  return array('actions' => $actions, 'modules' => $modules, 'info' => $patterns);
}

/**
 * Validates a pattern name.
 *
 * @param mixed $name
 * @TODO Doc.
 */
function patterns_parser_validate_pattern_name($name = NULL) {

  if (empty($name)) {
    return FALSE;
  }

  $pattern = array('/\.[^\.]*$/', '/[^a-zA-Z0-9_]/');
  $replacement = array('', '_');
  $name = preg_replace($pattern, $replacement, basename($name));

  $analysis = _patterns_parser_analyze_pattern_name($name);
  return (empty($analysis)) ? TRUE : FALSE;
}

/**
 * Analyzes a pattern name and return an array of errors messages.
 *
 * @param mixed $name
 * @TODO Doc.
 */
function _patterns_parser_analyze_pattern_name($name = NULL) {
  $msgs = array();

  if (empty($name)) {
    $msgs[] = t('No pattern name provided.');
    return $msgs;
  }

  if (preg_match('/[^a-zA-Z0-9_]/', $name)) {
    $msgs[] = t('You can only include letters, numbers, and underscores in the pattern identifier.');
  }

  if (preg_match('/^_/', $name)) {
    $msgs[] = t('You cannot start the pattern identifier with an underscore.');
  }

  return $msgs;
}



/**
 * Returns all the actions from a a given section
 * @param array $section
 * @param array $pattern the pattern to scan
 *
 */
function patterns_parser_retrieve_actions_from_section($section, $options = array()) {
  if (empty($section)) return FALSE;

  $actions = array();
  foreach ($section as $key => $value ) {
    $action = key($value);
    if (in_array($action, patterns_actions())) {
      if ($action !== PATTERNS_INCLUDE) {
        $actions[] = $section[$key];
      }
      // include
      else {
          $include = $value[PATTERNS_INCLUDE];
          if (patterns_parser_should_include_local($include, $options)) {
            // $actions is passed as reference
            patterns_parser_include_pattern($actions, $include['pattern'], $options);
//            $new_actions =  _patterns_parser_retrieve_actions_from_include($include, $options);
//            $actions = array_merge($actions, $new_actions);
          }
      }

    }
  }
  return $actions;
}

/**
 * Returns all the actions from a pattern array.
 * Sections are not preserved.
 *
 * @param $include the pattern to scan
 * @param array $sections include only these sections (not used for now)
 *
 */
function _patterns_parser_retrieve_actions_from_include($include, $options, $sections = NULL) {
  $actions = array();
  if (!isset($include['pattern']) || empty($include['pattern'])) {
    return $actions;
  }

  // if pattern is an id or a name,
  // try to load it from the database
  if (!is_array($include['pattern'])) {
    $p = patterns_get_pattern($include['pattern']);
    if (!$p) {
      drupal_set_message(t('Failed to include pattern %pattern'), array('%pattern' => $include['pattern']));
      return $actions;
    }
    $include['pattern'] = $p->pattern;
  }

  return patterns_parser_extract_all_actions($include['pattern'], $options, $sections);
}

/**
 * Inserts the actions of a pattern into another one.
 * Sections of the included patterns are not preserved, and all its actions
 * are appended at the end of the first parameter
 *
 * @param array &$actions the including pattern, or a subset of actions of it
 * @param array $include the included pattern
 * @param array $options the include options for nested patterns included
 *  in the included patterns.
 * @param array $sections array containing the name of the sections,
 *  from which extract the actions. If NULL, all the sections will be
 *  scanned.
 *
 */
function patterns_parser_include_pattern(&$actions, $include, $options = array(), $sections = NULL) {
  if (empty($include)) return $actions;
  if (is_array($include) && isset($include['pattern'])) {
    // take the code / id / name
    $include = $include['pattern'];
  }
  if (!patterns_install_modules_in_pattern($include)) {
    drupal_set_message(t('Failed to load the necessary modules for the included pattern. Its actions will not be executed.'));
    return $actions;
  }
  // Success
  $new_actions = patterns_parser_extract_all_actions($include, $options);
  $actions = array_merge($actions, $new_actions);
}

/**
 * Determines whether an pattern should be included based on the
 * local configuration (the pattern code itself).
 *
 * If a pattern id is specified in the associative array $options,
 * it also takes that into consideration, otherwise uses the default
 * include settings.
 *
 *  @see patterns_parser_should_include
 *  @see _patterns_parser_merge_default_include_options
 */
function patterns_parser_should_include_local($include, $options = array()) {
  // ATTACHED
  $options = _patterns_parser_merge_default_include_options($options);

  if ($options['mode'] === PATTERNS_INCLUDE_ATTACHED) {
    if (!is_array($include['pattern'])) {
      if ($options['verbose']) {
        drupal_set_message(t('A pattern was not included because lookup in the database is prohibited by the current configuration.'), 'status');
      }
      return FALSE;
    }
  }
  // OTHER MODES
  elseif (!empty($include) && isset($include['run'])) {

     // then we check if something is specified in the pattern file
    if (patterns_parser_is_valid_include_mode($include['run'])) {
      if ($include['run'] === PATTERNS_INCLUDE_ATTACHED) {
        return patterns_parser_should_include_local($include, array('mode' => PATTERNS_INCLUDE_ATTACHED));
      }

      $options = array_merge($options, array('mode' => $include['run']));
      return patterns_parser_should_include(NULL, $options);

    }
  }

  // RETURN GLOBAL OPTION IF WE COULD NOT DO BETTER
  return $options['include'];
}

/**
 * Determines if a pattern should be included or not.
 * The following conditions are checked in order:
 *
 *  - if a 'run' attribute is associated with the include tag
 *  - if $mode parameter was passed
 *  - the default include mode from system variables
 *
 * @param mixed $pattern the including pattern, identified by its id or its name, or
 *  the full object as loaded from the database. If the including
 * @param array $include the include tag from a pattern
 * @param mixed $mode a valid mode for sub-patterns execution
 *
 */
function patterns_parser_should_include($pattern = NULL, $options = array()) {

  $options = _patterns_parser_merge_default_include_options($options);

  $pattern = is_null($pattern) ? $options['pattern'] : $pattern;
  $updated = $options['updated'];
  $enabled = $options['enabled'];
  $verbose = $options['verbose'];
  $mode    = $options['mode'];

  // If we set $mode programmatically, that has the highest priority
  // otherwise fallback on default configuration
  if (!patterns_parser_is_valid_include_mode($mode)) {
      $mode = variable_get('patterns_default_include_mode', PATTERNS_INCLUDE_NEVER);
  }

  // Do we want to include it into a specific pattern?
  // If so we need to check its status from the database
  if (!is_null($pattern)) {
      $p = _patterns_db_get_pattern($pattern);
      if ($p) {
        $updated = ($p->updated >= $p->enabled) ? TRUE : FALSE;
        $enabled = ($p->status == PATTERNS_STATUS_ENABLED) ? TRUE : FALSE;
      }
  }

  switch ($mode) {
    case PATTERNS_INCLUDE_ALWAYS:
      break;

    case PATTERNS_INCLUDE_ATTACHED:
      // This option is checked by patterns_parser_should_include_local
      break;

    case PATTERNS_INCLUDE_FIRSTRUN:
      // Only run on first run.
      if ($enabled) {
        if ($verbose) {
          drupal_set_message(t('A pattern was not included because the pattern was set to execute only on the first run.'), 'status');
        }
        return FALSE;
      }
      break;

    case PATTERNS_INCLUDE_UPDATE:
      // Only run on pattern update.
      if (!$updated) {
        if ($verbose) {
           drupal_set_message(t('A pattern was not included because the pattern was set to execute only on pattern update.'), 'status');
        }
        return FALSE;
      }
      break;

    case PATTERNS_INCLUDE_FIRSTRUN_OR_UPDATE:
      // Only run on first run or pattern update.
      if (!$enabled || !$updated) {
        if ($verbose) {
          drupal_set_message(t('A pattern was not included because the pattern was set to execute only on first run or update.'), 'status');
        }
        return FALSE;
      }
      break;

    case PATTERNS_INCLUDE_NEVER:
    default:
      if ($verbose) {
        drupal_set_message(t('A pattern was not included because that is forbidden by the current configuration.'), 'status');
      }
      return FALSE;
      break;
  }

  return TRUE;
}

function _patterns_parser_merge_default_include_options($options) {
  $defaults = array('include' => FALSE,
                    'enabled' => FALSE,
                    'updated' => FALSE,
                    'verbose' => FALSE,
                    'pattern' => NULL,
                    'mode' => NULL,
  );

  return array_merge($defaults, $options);
}

/**
 * Returns TRUE if the passed parameter is a valid flag
 * to configure Patterns behaviors with the 'include' tag.
 *
 */
function patterns_parser_is_valid_include_mode($mode = NULL) {
    if (is_null($mode)) return FALSE;

    $modes = array( PATTERNS_INCLUDE_NEVER,
                  PATTERNS_INCLUDE_FIRSTRUN,
                  PATTERNS_INCLUDE_UPDATE,
                  PATTERNS_INCLUDE_FIRSTRUN_OR_UPDATE,
                  PATTERNS_INCLUDE_ATTACHED,
                  PATTERNS_INCLUDE_ALWAYS,
  );

  return (in_array($mode, $modes)) ? TRUE : FALSE;
}

function patterns_parser_build_include_options($pid, $run_mode = NULL) {

  $run_mode = patterns_get_include_mode($run_mode);
  $options = array('mode' => $run_mode);

  return array('pattern' => $pid,
               'include' => patterns_parser_should_include($pid, $options),
               'mode' => $run_mode,
  );
}

/**
 * Extract all the actions from a pattern and returns them as an array.
 *
 * @param mixed $pattern A pattern object, an array representing
 *  the pattern object, a numeric id or alphanumeric name of
 *  the pattern as it is in the database
 *
 * @param array $sections array containing the name of the sections,
 *  from which extract the actions. If NULL, all the sections will be
 *  scanned.
 *
 */
function patterns_parser_extract_all_actions($pattern, $options, $sections = NULL) {
  $actions = array();
  $pattern = _patterns_db_get_pattern_array($pattern);
  if (!$pattern) return $actions;
  if (isset($pattern['pattern'])) {
    $pattern = $pattern['pattern'];
  }
  // Loop through the sections of the included pattern
  foreach ($pattern as $section => $value) {
    if (in_array($section, array('info', 'modules'))) continue;
    if (!is_null($sections) && !in_array($section, $sections)) continue;
    $newactions = patterns_parser_retrieve_actions_from_section($value, $options);
    $actions = array_merge($actions, $newactions);
  }

  return $actions;
}

/**
 * Extract all the values of the modules tag from a pattern and
 * returns them as an array.
 *
 */
function patterns_parser_extract_modules($pattern) {
  $modules = array();
  if (!empty($pattern) && isset($pattern['modules'])) {
    $modules = (!is_array($pattern['modules'])) ? array($modules)
                                                : $pattern['modules'];
  }
  return $modules;
}
