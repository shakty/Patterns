<?php

function theme_patterns_form_helper_menu($forms) {
  $output = '<ul class="patterns-form-menu">';

  foreach ($forms as $form_id => $values) {
    $output .= '<li class="patterns-form-menu-item">' . $form_id . '</li>';
  }

  $output .= '</li>';

  return $output;
}

function theme_patterns_form_helper($form_id, $values) {

  $output = '<div class="patterns-form" id="patterns-form-' . $form_id . '">';

  $output .= '<div class="patterns-form-title">' . t('Form values for %key', array('%key' => $form_id)) . '</div>';

  foreach ($values as $key => $value) {
    $output .= '<div class="patterns-form-item"><div class="patterns-form-key">' . $key . ' => </div>';
    $output .= '<div class="patterns-form-value">' . print_r($value, TRUE) . '</div></div>';
  }

  $output .= '</div>';

  return $output;
}

/**
 * Implements hook_theme().
 */
function patterns_theme() {
  return array(
    'patterns_form_helper' => array(
      'arguments' => array('form_id' => NULL, 'values' => NULL),
  ),
    'patterns_form_helper_menu' => array(
      'forms' => NULL
  )
  );
}