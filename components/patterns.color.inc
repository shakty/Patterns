<?php
/*
 * @file
 * Patterns component for pathauto.
 */
function color_patterns() {
  $files = array('modules/system/system.module','modules/system/system.admin.inc');

  $actions['color'] = array(
    'descr' => t('Setting color'),
    PATTERNS_MODIFY => array('system_theme_settings'),
    PATTERNS_FILES => $files,
  );

  return $actions;

}

function color_patterns_actions() {
  return array(
    'system_theme_settings' => t('color: Edit scheme color settings'),
  );
}

// Prepare data for processing
function color_patterns_prepare($action, $tag, &$data = NULL) {
  if (!isset($data['scheme'])) {
    $data['scheme'] = '';
  }
  return patterns_results();
}

// Validate the values for an action before running the pattern
function color_patterns_validate($action, $tag, &$data = NULL) {
  $status = PATTERNS_SUCCESS;
  $msg = '';
  foreach ($data['palette'] as $key => $color) {
    if (!preg_match('/^#([a-f0-9]{3}){1,2}$/iD', $color)) {
      $msg = t('%name must be a valid hexadecimal CSS color value.', array('%name' => $data['palette'][$key]));
      break;
    }
  }
  return patterns_results($status, $msg);
}

// Prepare for valid processing of this type of component
function color_patterns_build($action, $tag, &$data = NULL, $form_id) {

  return patterns_results(PATTERNS_SUCCESS,    t('Execution successful'), $data);
}

// Build a patterns actions and parameters
function color_patterns_params($action, $form_id, &$data = NULL, &$a) {
  $result = array($data['theme']);

  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

function color_patterns_cleanup($action, $tag, &$data = NULL) {
  return patterns_results();
}

// Return which callback functions to actually use.
function color_patterns_callbacks($action, $tag, &$data = NULL) {
  $desc = color_patterns();
  $result = $desc[$tag][$action];
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}
