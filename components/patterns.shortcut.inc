<?php
/**
 * @file
 * Patterns component for shortcut.
 */

/**
 * hook_patterns()
 */
function shortcut_patterns() {
  $files = array('modules/shortcut/shortcut.admin.inc');
  
  $actions['shortcut_set'] = array(
    PATTERNS_INFO => t('create/delete/modify shortcut set'),
    PATTERNS_CREATE => array('shortcut_set_add_form'),
    PATTERNS_MODIFY => array('shortcut_set_edit_form'),
    PATTERNS_DELETE => array('shortcut_set_delete_form'),
    PATTERNS_FILES => $files,
    );
  $actions['shortcut_link'] = array(
    PATTERNS_INFO => t('add a new link to one shortcut_set'),
    PATTERNS_CREATE => array('shortcut_link_add'),
    PATTERNS_MODIFY => array('shortcut_link_edit'),
    PATTERNS_DELETE => array('shortcut_link_delete'),
    PATTERNS_FILES => $files,
    );
  $actions['shortcut_set_user'] = array(
    PATTERNS_INFO => t('one user get one shortcut_set'),
    PATTERNS_MODIFY => array('shortcut_set_switch'),
    PATTERNS_FILES => $files,
    );
  
  return $actions;
}

function shortcut_patterns_actions() {
  return array(
    'shortcut_set_add_form' => t('Shortcut: add a new shortcut set'),
    'shortcut_set_edit_form' => t('Shortcut: change the name a shortcut set'),
    );
}

/**
 * hook_patterns_prepare()
 */
function shortcut_patterns_prepare($action, $tag, &$data = NULL) {
  if ($tag == 'shortcut_set') {
    if ($action == PATTERNS_CREATE ) {
      //'new' is the form's textfield.
      $data['new'] = $data['shortcut_set_name'];
    }
  }

  return patterns_results();
}

/**
 * hook_patterns_validate()
 */
function shortcut_patterns_validate($action, $tag, &$data = NULL) {

  $status = PATTERNS_SUCCESS;
  $msg = '';

  //for shotcut set edit (change the short set's name)
  if ($action == PATTERNS_MODIFY && $tag == 'shortcut_set') {
    $set = db_select('shortcut_set', 'ss')
      ->fields('ss',array('set_name'))
      ->condition('title', $data['oldname'])
      ->execute()
      ->fetchField();
    if ($set == FALSE) {
      $status = PATTERNS_ERR;
      $msg = 'Shortcut Error:there is no oldname shortcut set';
      return patterns_results($status, $msg);
    } else {
      //include a module file , is this all right?
      module_load_include('module', 'shortcut');
      $shortcutsetobj = shortcut_set_load($set);
      $data['shortcut_set_obj'] = $shortcutsetobj;
    }
  }
  
  return patterns_results($status, $msg);
}

/**
 * hook_patterns_callbacks()
 */
function shortcut_patterns_callbacks($action, $tag, &$data = NULL) {
  // for shortcut set edit(change shortcut set's name) ,the callback is not in form mothod.
  if ($tag == 'shortcut_set' && $action == PATTERNS_MODIFY ) {
    $result = array(
      'shortcut_set_modify'
      );
  } else {
  // for others, the callback is in form mothod.
    $allform = shortcut_patterns();
    $result = $allform[$tag][$action];
  }
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful(in callbacks function)'), $result);
}

/**
 * shortcut set edit's callback function, operate on the database.
 * the funciton containing "shortcut_set_edit_form" in shortcut module have three parms:
 *     $form
 *     &$form_state
 *     $shortcut_set
 * I don't know how to manager the third parm "$shortcut_set", so I directly operate on the database.
 */
function shortcut_set_modify($action, &$data) {
  $data['shortcut_set_obj']->title = $data['newname'];
  module_load_include('module', 'shortcut');
  shortcut_set_save($data['shortcut_set_obj']);
  drupal_set_message(t('Updated %oldname to %newname', array('%oldname' => $data['oldname'], '%newname' => $data['newname'])));
}

/**
 * hook_patterns_build()
 */
function shortcut_patterns_build($action, $form_id, &$data = NULL, &$a) {
  //include the form's relatied function
  module_load_include('inc', 'shortcut', 'shortcut.admin');
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful(in build function)'), $data);
}

/**
 * hook_patterns_params()
 */
function shortcut_patterns_params($action, $form_id, &$data = NULL, &$a) {
  return patterns_results();
}

/**
 * hook_patterns_cleanup()
 */
function shortcut_patterns_cleanup($action, $tag, &$data = NULL) {
  return patterns_results();
}
