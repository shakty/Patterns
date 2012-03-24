<?php
/**
 * @file
 * Displays a form of necessary input configuration to the user
 * 
 */

function patterns_first_install_page() {
	

	
	
	
	//$form['#submit'] = 'patterns_first_install_submit';
	
	//	return drupal_render($form);

	$out = drupal_get_form('patterns_first_install');	
	return $out;
}


function patterns_first_install($form, &$form_state) {
	
	$intro = 'This is the first time you installed patterns. ';
	$intro.= 'This page gives you a brief guide to the main functionalities of Patterns and settings. TODO check which parsers are enabled, if components are enabled, path to drush, and other settings.';
	
	$form = array();
	
	$form['welcome'] = array (
    '#type' => 'fieldset',
    '#title' => t('Welcome to Patterns!'),
  );
  
	$form['welcome']['intro'] = array (
		'#prefix' => '<div>',
		'#suffix' => '</div><br/>',
		'#markup' => t($intro),
	);
	
	$form['welcome']['submit'] = array (
	  '#type' => 'submit',
    '#value' => t('Settings are OK'),
		'#suffix' => '<div>' . t('By pressing OK this page will not be shown any more. You can always modify your preferences from the \'Settings\' tab.') . '</div>',
	);
	
	return $form;
}

function patterns_first_install_validate($form, &$form_state){
	return TRUE;
}

function patterns_first_install_submit($form, &$form_state) {
	variable_set(PATTERNS_FIRST_INSTALL, FALSE);
	 menu_rebuild();
}