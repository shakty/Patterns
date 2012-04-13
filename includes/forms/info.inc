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
	$dirs = patterns_array_to_rows($dirs);

  $form['pd'] = array(
    '#type' => 'fieldset',
    '#title' => t('Patterns Directories'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  	'#description' => t('The following directories will be scanned for Patterns files.'),
  );

  $form['pd']['dirs'] = array(
    '#markup' => theme('table', array('rows' => $dirs, 'attributes' => array('class' => 'patterns-list'))),
  );

  return $form;
}

function patterns_info_components($form = array()) {

	$components = patterns_components_list();
	$components = patterns_array_to_rows($components);
	
  $form['cd'] = array (
    '#type' => 'fieldset',
    '#title' => t('Patterns Components'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  	'#description' => t('The following Patterns components were found and are currently available.'),
  );


  
  $form['cd']['components'] = array (
    '#markup' => theme('table', array('rows' => $components, 'attributes' => array('class' => 'patterns-list'))),
  );

  return $form;
}

function patterns_info_parsers($form = array()) {
	
	
	
	$parsers = patterns_parser_build_formats_index();
	
	foreach ($parsers as $key => $value) {
		$enabled = (patterns_utils_is_module_enabled($value)) ? t('Enabled') : t('Disabled');	
    $parsers[$key] = array($enabled, $key, $value);
  }
	
	$header = array (
		t('Status'), 
    t('Format'),
    t('Module'),
  );
	
  $form['parsers'] = array (
    '#type' => 'fieldset',
    '#title' => t('Patterns Parsers'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  	'#description' => t('Go to the !modules page to enable more Patterns parsers.', array('!modules' => l('modules', 'admin/modules'))),
  );

  $form['parsers']['parsers'] = array (
    '#markup' => theme('table', array('header' => $header, 
    																	'rows' => $parsers, 
  																		'empty' => t('No valid parser found / enabled'),
    																	//'attributes' => array('class' => 'patterns-list'),
  														)),
  );

  return $form;
}
