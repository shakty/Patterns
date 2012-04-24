<?php

/**
 * @file
 * Functions related to parsing pattern files.
 */

module_load_include('inc', 'patterns', 'includes/parser/scan');


/**
 * Register all available patterns parsers and returns an associative
 * array.
 *
 * Invokes all the modules implementing <hook_patterns_parser>.
 *
 * Modules implenting such hook are expected to return an associative
 * array of the type:
 *
 *  $parser = array();
 *  $parser['format'] = 'php;
 *  $parser['parser'] = 'patterns_phpparser';
 *  $parser['overwrite'] = TRUE; // (Optional) Default is TRUE
 *
 * Patterns will then call
 *  - <patterns_phpparser_parse()>
 *  - <patterns_phpparser_dump()>
 *  - <patterns_phpparser_load()>
 *
 * for performing parsing operations with the PHP format.
 *
 * @param bool $reset (optional) If TRUE, forces to rebuild the index.
 *
 * @return array $patterns_formats The associative array of
 *   available parsers.
 *
 */
function patterns_parser_build_formats_index($reset = TRUE) {

  static $patterns_formats;
  if (!$reset) {
    return $patterns_formats;
  }

  if (!is_array($patterns_formats)) {
    $patterns_formats = array();
  }

  $modules = module_implements('patterns_parser');
  foreach ($modules as $module) {
    $parser = module_invoke($module, 'patterns_parser');
    if (!is_array($parser) || !isset($parser['parser']) || !isset($parser['format'])) {
      continue;
    }

    $format = $parser['format'];
    $parser = $parser['parser'];

    $ow = (isset($parser['overwrite'])) ? isset($parser['overwrite'])
                                        : TRUE; // overwrite by default

    // If the parser already exists and we are not
    // allowed to overwrite it, contintue
    if (!$ow && patterns_parser_exists($format, FALSE)) {
      continue;
    }

    $patterns_formats[$format] = $parser;
  }

  return $patterns_formats;
}

/**
 * Returns an array of formats currently supported by
 * the patterns module.
 *
 * @param bool $reset (optional) If FALSE, it does not rebuild the index
 *   of available formats.
 *
 * @return array Array of supported file types.
 *
 */
function patterns_parser_get_formats($reset = TRUE) {
  return array_keys(patterns_parser_build_formats_index($reset));
}

/**
 * Checks whether there is at least one parser currently enabled.
 *
 * @return Boolean TRUE if there is at least one parser available.
 *
 */
function patterns_parser_ready() {
  $formats = patterns_parser_get_formats();
  return (empty($formats)) ? FALSE : TRUE;
}

/**
 * Checks wheter a parser is defined for a given format.
 *
 * @param mixed $format A string representing the format
 *
 * @return bool TRUE, if the parser is defined, FALSE otherwise.
 */
function patterns_parser_exists($format = PATTERNS_FORMAT_UNKNOWN, $reset = TRUE) {
  if (empty($format) || $format == PATTERNS_FORMAT_UNKNOWN) return FALSE;

  $patterns_formats = patterns_parser_build_formats_index($reset);

  if (!isset($patterns_formats[$format]) || empty($patterns_formats[$format])) {
    return FALSE;
  }
  return TRUE;
}

/**
 * Checks whether a parser is defined for a given format, and returns
 * the corresponding function name for the requested action.
 *
 * @param mixed $format A string representing the format.
 * @param mixed $action (optional) A string representing the desidered
 *   action to call. Default PATTERNS_PARSER_PARSE.
 *
 *
 * @return bool|mixed the function name if the parser exists, FALSE otherwise.
 *
 */
function patterns_parser_get_parser_function($format = PATTERNS_FORMAT_UNKNOWN,
                                              $action = PATTERNS_PARSER_PARSE) {

  if (!patterns_parser_exists($format)) return FALSE;

  $patterns_formats = patterns_parser_build_formats_index();
  return $patterns_formats[$format] . '_' . $action;
}


/**
 * Tries to parse a pattern string according to the specified format.
 * If succesfull returns the array representation of the pattern, if not
 * returns FALSE;
 *
 * @param mixed $pattern A string representation of the pattern, or a pattern
 *  array. In the latter case, the array is returned as it is, and the @param
 *  $format is ignored.
 * @param mixed $format The format against which parse the pattern string.
 *
 * @return bool|array The parsed pattern or FALSE.
 *
 */
function patterns_parser_parse($pattern, $format = PATTERNS_FORMAT_UNKNOWN) {
    if (empty($pattern)) return FALSE;
    if (is_array($pattern)) return $pattern;

    $parse_function = patterns_parser_get_parser_function($format);
    if (!$parse_function) {
      return FALSE;
    }

    return $parse_function($pattern);
}

/**
 * Tries to dump an array representing a pattern to a string, according to
 * the specified format.
 *
 * If succesfull returns the array representation of the pattern, if not
 * returns FALSE;
 *
 * @param mixed $pattern A string representation of the pattern, or a pattern
 *  array. In the latter case, the array is returned as it is, and the @param
 *  $format is ignored.
 * @param mixed $format The format against which parse the pattern string.
 * @param mixed $append (optional) A string to which the dump will be
 *   appended. Defaults NULL
 *
 * @return bool|mixed The string representation of the pattern or FALSE.
 *
 */
function patterns_parser_dump($pattern, $format = PATTERNS_FORMAT_UNKNOWN, $append = NULL) {
    if (empty($pattern)) return FALSE;
    if (is_string($pattern)) return $pattern;
    if (!is_array($pattern)) return FALSE;

    $dump_function = patterns_parser_get_parser_function($format, PATTERNS_PARSER_DUMP);
    if (!$dump_function) {
      return FALSE;
    }

    return $dump_function($pattern, $append);
}

/**
 * Tries to dump an array representing a pattern to a string, according to
 * the specified format.
 *
 * If succesfull returns the array representation of the pattern, if not
 * returns FALSE;
 *
 * @param mixed $pattern A string representation of the pattern, or a pattern
 *  array. In the latter case, the array is returned as it is, and the @param
 *  $format is ignored.
 * @param mixed $format The format against which parse the pattern string.
 * @param mixed $append (optional) A string to which the dump will be
 *   appended. Defaults NULL
 *
 * @return bool|mixed The string representation of the pattern or FALSE.
 *
 */
function patterns_parser_dump_comment($text, $format = PATTERNS_FORMAT_UNKNOWN, $append = NULL) {
    if (empty($text)) return FALSE;

    $dump_function = patterns_parser_get_parser_function($format, PATTERNS_PARSER_DUMP_COMMENT);
    if (!$dump_function) {
      return FALSE;
    }

    return $dump_function($text, $append);
}

/**
 * Tries to load a pattern from a file according to the specified format.
 * If succesfull returns the array representation of the pattern, if not
 * returns FALSE;
 *
 * @param mixed $path The path to the pattern file
 * @param mixed $format The format against which parse the loaded pattern.
 *
 * @return bool|array The parsed pattern or FALSE.
 *
 */
function patterns_parser_load($path, $format = PATTERNS_FORMAT_UNKNOWN) {
    if (empty($path)) return FALSE;

    $load_function = patterns_parser_get_parser_function($format, PATTERNS_PARSER_LOAD);
    if (!$load_function) {
      return FALSE;
    }

    return $load_function($path);
}

/**
 * Checks whether a string is a valid action name.
 *
 * Calls patterns_actions() to determine the list of
 * valid action names.
 *
 * @param mixed $action The string to check.
 *
 * @return Boolean TRUE if the string is a valid action
 *   name
 */
function patterns_parser_is_valid_action_name($action = NULL) {
  if (is_null($action)) return FALSE;
  $actions = patterns_actions();
  if (in_array($action, $actions)) {
    return TRUE;
  }
  else {
    return FALSE;
  }
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

  $scan = patterns_scan_pattern($pattern);
  return _patterns_scan_validate_patternscan($scan);
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

  if (!$pattern) {
    print t('Pattern could not be parsed. Please check pattern format.');
    exit;
  }

  $scan = patterns_scan_pattern($pattern);

  if (_patterns_scan_validate_patternscan($scan)) {
    $count = count($scan['other_sections']) + $scan['info'] + $scan['modules'];
    $out = t('This pattern is valid. !count sections found ->', array('!count' => $count)) . ' ';

    $out .= 'info ';
    $out .= ($scan['modules']) ? 'modules ' : '' ;

    foreach ($scan['other_sections'] as $key => $value) {
      $out .= $key . ' ' . $value;
    }
  }
  else {
    $errors = implode('<br/> ', _patterns_scan_analyze_patternscan($scan));
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
  $scan = patterns_scan_pattern($pattern->pattern);
  // This will display errors if there are any.
  if (!_patterns_scan_validate_patternscan($scan, TRUE)) return FALSE;

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
 *  @see patterns_parser_should_include()
 *  @see _patterns_parser_merge_default_include_options()
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
