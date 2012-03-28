<?php
/**
 * @file
 * Functions, forms related to display general information
 * about the Patterns module as a whole.
 */


function patterns_info_page() {

  $form = array();
  
  $text = 'General Information about the state of the Patterns module.';
	$title = 'Patterns Info';
  patterns_forms_add_page_header($form, $title, $text);
  
  $form = patterns_info_directories($form);
  $form = patterns_info_components($form);
  $form = patterns_info_parsers($form);

  return drupal_render($form);
}

function patterns_info_directories($form = array()) {
  $dirs = patterns_path_get_patterns_dirs();

  foreach ($dirs as $key => $value) {
    $dirs[$key] = array(($key+1), $value);
  }

  $form['pd'] = array(
    '#type' => 'fieldset',
    '#title' => t('Patterns Directories'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['pd']['dirs'] = array(
    '#markup' => theme('table', array('rows' => $dirs, 'attributes' => array('class' => 'patterns-list'))),
  );

  return $form;
}

function patterns_info_components($form = array()) {

  $form['cd'] = array(
    '#type' => 'fieldset',
    '#title' => t('Patterns Components Directories'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['cd']['dirs'] = array(
    '#markup' => 'TODO',
  );

  return $form;
}

function patterns_info_parsers($form = array()) {

  $form['parsers'] = array(
    '#type' => 'fieldset',
    '#title' => t('Patterns Parsers'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['parsers']['parsers'] = array(
    '#markup' => 'TODO',
  );

  return $form;
}
