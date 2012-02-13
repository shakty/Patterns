<?php
/**
 * @file
 * Functions related to exporting patterns.
 */

function patterns_export($form, &$form_state) {
  // stub
  $form = array();
  $form['container'] = array(
    '#type' => 'fieldset',
    '#title' => t('stub Form'),
    '#description' => t('stub form')
  )  ;
  return $form;
}
