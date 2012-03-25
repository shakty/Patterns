<?php

/**
 * @file
 *
 * API for writing pattern files.
 *
 */

/**
 * Returns an array containing all the reserved words
 * for a Patterns.
 * 
 * Such words are not allowed to be used as a section heading.
 * 
 * @return Array
 *   The array of reserved words.
 */
function patterns_api_reserved_words() {
	return array(
		PATTERNS_TAG,
		PATTERNS_SECTION_INFO,
		PATTERNS_SECTION_MODULES,
		PATTERNS_CREATE,
		PATTERNS_MODIFY,
		PATTERNS_DELETE,
		PATTERNS_INCLUDE,
	);
}

/**
 * Tells whether a string is a Patterns reserved word
 * 
 * @param mixed $word
 * 	 The string to check.
 * 
 * @return Boolean,
 *   TRUE, if it is a reserved word
 */
function patterns_api_is_reserved_word($word = NULL) {
	if (is_null($word)) return FALSE;
	return (in_array($word, patterns_api_reserved_words())) ? TRUE
																													: FALSE;
}

/**
 * @see patterns_get_tagmodules_index
 * @see patterns_api_build_moduletags_index
 * 
 */
function patterns_api_build_tagmodules_index($tag = NULL, $data = NULL, $key = NULL) {
	return patterns_get_tagmodules_index($tag, NULL, $key);
}
/**
 * Builds up an associative array of modules and exposed tags.
 * 
 * @param mixed $module
 *   (optional) If set, only information about this specific module is returned.
 *   Defaults NULL.
 * @param mixed $key (optional) If set, only the values for that key 
 * 	 from all modules are returned. E.g. (PATTERNS_CREATE, PATTERNS_MODIFY, etc.)
 * 	 Defaults NULL. 
 *
 * @return array $moduletags The tagmodules index built according to the parameters.
 * 
 * @see patterns_api_build_tagmodules_index
 */
function patterns_api_build_moduletags_index($module = NULL, $data = NULL, $key = NULL) {
	 static $moduletags;

  // Index not yet built
  if (empty($moduletags)) {
  
    $moduletags = array();
    foreach (module_implements('patterns') as $component) {
      $tags = module_invoke($component, 'patterns', $data);
      $moduletags[$component] = $tags;
    }
  }

  debug($moduletags);
  
  // Part of tag
  if (!is_null($module) && !is_null($key)) {
    return $moduletags[$module][$key];  
  }
  // Full tag
  if (!is_null($module)) {
    return $moduletags[$module];
  }
  
  // All the values of key=$key from all tags
	if (!is_null($key)) {
		$out = array();
		foreach ($moduletags as $module => $value) {
			if (isset($value[$key])) {
				$out[$module] = $value[$key];
			} 
		}
    return $out;
  }
  
  // All values from all tags
  return $moduletags;
}


/**
 * Checks whether a string is a valid component name.
 * 
 * @param mixed $component 
 * 	 The string with the name of the component
 * 
 * @return Boolean 
 * 	 TRUE, if the string is a valid component name
 * 
 * @see patterns_api_build_moduletags_index
 */
function patterns_api_is_valid_component_name($component = NULL) {
	if (is_null($component)) return FALSE;
	$idx = patterns_api_build_moduletags_index();
	return (isset($idx[$component])) ? TRUE : FALSE;
}


/**
 * Adds / Replaces an Info section to a pattern. 
 * 
 * Missing properties are options are automatically filled in with
 * standard values.
 * 
 * @param array $info (optional)
 * 	 An associative array of properties for the info section.
 * @param array $pattern (optional)
 *   An array representing a pattern file.
 * @param Bool $ow (optional)
 *   If TRUE, overwrites existing Info sections.
 */
function patterns_api_add_info_section($info = array(), &$pattern = array(), $ow = FALSE) {
	if (isset($pattern[PATTERNS_SECTION_INFO]) && !$ow) return FALSE;
	
  global $user;
  global $base_root;
  
  $pattern[PATTERNS_SECTION_INFO] = array();

  $pattern[PATTERNS_SECTION_INFO]['title']          = (isset($info['title'])) ? $info['title'] : 'Untitled Pattern';
  $pattern[PATTERNS_SECTION_INFO]['category']       = (isset($info['category'])) ? $info['author'] : 'General';
  $pattern[PATTERNS_SECTION_INFO]['description']    = (isset($info['description'])) ? $info['description'] : 'No description';
  $pattern[PATTERNS_SECTION_INFO]['version']    		= (isset($info['version'])) ? $info['version'] : '1.0';
  $pattern[PATTERNS_SECTION_INFO]['core']           = (isset($info['core'])) ? $info['core'] : 'x.y';
  $pattern[PATTERNS_SECTION_INFO]['author']         = (isset($info['author'])) ? $info['author'] : $user->name;
  $pattern[PATTERNS_SECTION_INFO]['author_email']   = (isset($info['author_email'])) ? $info['author_email'] : $user->mail;
  $pattern[PATTERNS_SECTION_INFO]['author_website'] = (isset($info['author_website'])) ? $info['author_website'] : $base_root;

  return $pattern;
}


/**
 * Initializes an empty section in a pattern file.
 * 
 * Checks whether the section name is valid before adding it.
 * 
 * @param mixed $section
 * 	 A string representing the pattern name
 * @param array $pattern (optional)
 *   The associative array representing a pattern
 * @param Bool $ow (optional)
 * 	 If TRUE, an existing section with the same name will be 
 * 	 overwritten. Defaults FALSE. 
 */
function patterns_api_add_section($section = NULL, &$pattern = array(), $ow = FALSE) {
	if (is_null($section)) return FALSE;
	if (patterns_api_is_reserved_word($section)) return FALSE;
	if (isset($pattern[$section]) && !$ow) return FALSE;
	
	$pattern[$section] = array();
	return $pattern;
}

/**
 * Extract the current state of a form.
 * Enter description here ...
 * @param unknown_type $form
 * @param unknown_type $args
 * @param unknown_type $action
 */
function patterns_api_extract_actions($form, $args = array(), $action = PATTERNS_MODIFY) {

	// Move this inside the Macro module
	// Macro should check that only one user is using that
	$form_state = array();
	$form_state['values'] = array();
	variable_set('macro_submissions', array());

	
	$form_state['build_info']['args'] = $args;
	drupal_form_submit($form, $form_state);
	
	$result = macro_get_macro();
	
	variable_set('macro_enabled', FALSE);
	
	$out = array();
	foreach ($result as $a) {
		$out[] = array ($action => $a);	
	}
	
	
	return $out;
}

/**
 * Helper function to create the associative array of returned values from
 * an invoked function.
 *
 * @param mixed $status One of [PATTERNS_SUCCESS, PATTERNS_WARN, PATTERNS_ERR]. Required.
 * @param array $msg A message.
 * @param mixed $result Any kind of additional data.
 *
 * @return array An associative array of the input values with keys 'status', 'msg', 'result'.
 */
function patterns_results($status = PATTERNS_SUCCESS, $msg = NULL, $result = NULL) {
  if ($msg === NULL) {
    $msg = t('Execution successful');
  }
  return array('status' => $status,
                'msg' => $msg,
                'result' => $result,
              );
}

/**
 * Checks if a (supposed) array is actually correctly formatted as a patterns_results().
 *
 * @param mixed $results The array to check.
 *
 * @return bool TRUE if correct, FALSE otherwise.
 * @see patterns_results()
 */
function _patterns_is_patterns_results($results) {
  // 'status' is the only mandatory field.
  if (!isset($results['status'])) {
    return FALSE;
  }

  return TRUE;
}


/**
 * Returns an array of allowed actions, as defined in patterns.module.
 *
 * @return array Allowed actions.
 */
function patterns_actions() {
  return array(PATTERNS_CREATE => PATTERNS_CREATE,
               PATTERNS_MODIFY => PATTERNS_MODIFY,
               PATTERNS_DELETE => PATTERNS_DELETE,
               PATTERNS_INCLUDE => PATTERNS_INCLUDE,
              );
}
