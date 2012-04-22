<?php
/**
 * @file
 * Functions, forms related to the Patterns editor.
 */


/**
 * @TODO Doc.
 */
function patterns_form_helper($form, $form_state) {
  static $form_id;

  if (!$form_id && $form_state['submitted'] && !form_get_errors()) {
    $form_id = $form_state['values']['form_id'];
    $_SESSION['patterns_form_helper'] = array('form_id' => $form_id, 'form_values' => $form_state['values']);
  }

  return $form;
}

