<?php
/*
 * @file
 *
 * API for writing pattern files.
 *
 */

/**
 * @see patterns_api_build_tagmodules_index
 * 
 * Enter description here ...
 * @param unknown_type $tag_name
 * @param unknown_type $data
 * @param unknown_type $id
 */
function patterns_api_build_tagmodules_index($tag_name = NULL, $id = NULL) {
	return patterns_get_tagmodules_index($tag_name, NULL, $id);
}

function patterns_api_extract($form, $args = array()) {


	// Move this inside the Macro module
	// Macro should check that only one user is using that
	$form_state = array();
	$form_state['values'] = array();
	variable_set('macro_submissions', array());

	
	$form_state['build_info']['args'] = $args;
	drupal_form_submit($form, $form_state);
	
	// TODO CleanUp
	
	return macro_get_macro_yaml();
           
  
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
