<?php
/**
 * @file
 * Importing Patterns from a file or using an URL.
 */

function _patterns_extract_goto($form, &$form_state, $step) {
  // MODULE SELECTED
  
	if ($step == 1) {
		return patterns_extract_page1($form, $form_state);
	}

	
  if ($step == 2) {
  	
  	// TODO: We should check if the users selects more options, and if the functions
  	// require extra parameters. For later, not we just continue.
  	
  	return $form; 
  	
  	
  	$tm_index = patterns_api_build_tagmodules_index();
  	$tag = $form_state['values']['options'];
  	$func_id = $form_state['values'][$tag];
  	
  	// Check if the function wants extra parameters
  	// if (isset($tm_index[$tag]) && isset($tm_index[$tag][PATTERNS_EXPORT][$func_id])){
  	//	if (isset($tm_index[$tag]))
  	//}

  	//if (isset($form_state['values']['module']) && !is_int($form_state['values']['module']))  {
		// return patterns_extract_page2($form, $form_state);
  	//}
  }	
  
	
	// FORM SELECTED
	/*  
  if ( $step == 3 && !isset($form_state['values']['content'])) {
  	return patterns_extract_page3($form, $form_state);
  }

  // show pattern import form
  if ($step = 4) {
		return patterns_extract_page4($form, $form_state);
  }
  */
  
  // step > 5 Continue, submit form
  return $form;
}

/**
 * Checks the form_state and returns the current step of the form.
 * 
 * 
 * @param array $form_state
 * @param bool $validate (optional)
 *   If TRUE, the assumes that function is called from a validation
 *   environment, and decrements by 1 the current value of the step
 * 
 * @return mixed|Bool
 * 	 The current value of the step, or FALSE if it fails to determine it
 */
function _patterns_extract_check_step(&$form_state, $validation = FALSE) {
 	
	$step = FALSE;
	
	if (isset($form_state['values']['all']) && $form_state['values']['all'] == 0) {
    $step = 6; // EXTRACT ALL
  }
  /*
	elseif (isset($form_state['values']['content'])) {
    $step = 4;
  }
	elseif (isset($form_state['values']['module_forms'])) {
    $step = 3;
  }
  */
	elseif (isset($form_state['values']['options'])) {
    $step = 2;
  }
	elseif (!isset($form_state['values']['step'])) {
    $step = 1;
  }
  
  if ($validation && is_numeric($step)) {
  	$step = $step - 1;
  }
  
  $form_state['values']['step'] = $step;
  return $step;
}

function _patterns_extract_if_invalid_step_goback($step = 0,
				                                          $back = '/admin/patterns/extract',
				                                          $options = array()) {
 	
	// TODO: Adjust
  if ($step < 1 || $step > 10) {
  	drupal_set_message(t('Extraction could not proceed to the next step. Step %id not found.', array('%id' => $step)), 'error');
  	drupal_goto($back);
  }
  return $step;
}


/**
 * Forms to create a yaml file based on a module
 * longer description:
 * Displays forms to choose a module (which implements the pattern hooks) and
 * the forms which we get from module_patterns,
 * then submits the forms and captures the form submissions with the Macro module.
 * From this, yaml file gets generated
 */

function patterns_extract($form, &$form_state) {
 	
 	$step = _patterns_extract_check_step($form_state);
	_patterns_extract_if_invalid_step_goback($step);
	$form['step'] = array(
     '#type' => 'hidden',
     '#title' => t('Pattern Identifier'),
     '#description' => t('step'),
     '#value' => $step,
  );
	
  //debug($step);
  
  //drupal_add_js('misc/ajax.js');
  $name = isset($form_state['values']['name']) ?  $form_state['values']['name'] : 'name not set';
  
  patterns_io_load_components();

  return _patterns_extract_goto($form, $form_state, $step);

}

function patterns_extract_page1($form, &$form_state) {

	
	$tm_index = patterns_api_build_tagmodules_index(null, null, PATTERNS_EXPORT);
	
	if (count($tm_index) == 0) {
		  drupal_set_message(t('The components currently installed do not allow automatic extraction of the configuration of the web site'), 'error');
			
			$form['disabled'] = array (
				'#markup' => l(t('Click here to back'), 'admin/patterns'),
			);
				
		return $form;
	}
	
	 $form['all'] = array (
		'#type' => 'fieldset',
	  '#title' => 'Options',
  	'#collapsible' => TRUE,
    '#collapsed' => TRUE, // TODO set to TRUE after debugging
	);
	
	$form['all']['all'] = array (
		'#type' => 'fieldset',
	  '#title' => 'I want to extract configuration from single components (Experimental)',
  	'#type' => 'checkbox',
	);
   	
  $form['all']['ext'] = array (
		'#type' => 'fieldset',
	  '#title' => 'Components with automatic extraction enabled',
  	//'#tree' => TRUE,
  	'#collapsible' => TRUE,
    '#collapsed' => TRUE, // TODO set to TRUE after debugging
  	'#states' => array(
		     	 'visible' => array(   // action to take.
		        ':input[name="all"]' => array('checked' => TRUE),
		      ),
	  		),
	);
	
  foreach ($tm_index as $module => $tags) {
  
  	$form['all']['ext'][$module] = array (
	    '#type' => 'fieldset',
	    '#title' => $module,
  		'#collapsible' => TRUE,
  		'#collapsed' => FALSE, // TODO set to TRUE after debugging
	  );
	  
	  /*
	  $form['all']['ext'][$module]['module'] = array(
	    '#type' => 'hidden',
	  	'#default_value' => $module,
	  ); 
	  */
	  
	  $form['all']['ext'][$module]['options'] = array(
	    '#type' => 'radios',
	    '#title' => t('Extraction options'),
	    '#options' => array_combine(array_keys($tags), array_keys($tags)),
	  ); 
	  
	  foreach ($tags as $tag => $forms) {

	  	/*
	  	$form['all']['ext'][$module][$tag]['tag'] = array(
	    	'#type' => 'hidden',
	  		'#default_value' => $tag,
	  	); 
	  	*/
	  	
	  	$form['all']['ext'][$module][$tag]['options'][$tag] = array(
	  	//$form['all']['ext'][$module][$tag]['options'] = array(
	      '#type' => 'radios',
	  		'#title' => $tag,
	    	'#options' => array_combine(array_keys($forms), array_keys($forms)),
	  		'#states' => array(
		     	 'visible' => array(   // action to take.
		        ':input[name="options"]' => array('value' => $tag),
		      ),
	  		),
	  	);
	  }
	  
	  
	  //_patterns_extract_add_ajax_to_options($form['ext'][$module]);
	  
	  // TODO: do we need this?
	  $form['all']['ext']['module'] = array(
	    '#type' => 'hidden',
	  	'#default_value' => $module,
	  ); 
	  
  }   
  
  patterns_forms_get_info_section($form, array(), array('collapsed' => TRUE));
  
  patterns_forms_get_formats_selector($form,
  																						patterns_forms_get_default_format(),
  																						'Extract data in this format',
  																						FALSE
  );
  
  patterns_forms_get_name_selector($form);
  
  $form['submit'] = array(
  	'#type' => 'submit',
    '#value' => t('Extract'),
  );
  $form['#validate'][] = 'patterns_extract_validate';
  
  return $form;
}

function patterns_extract_submit($form, &$form_state) {
	
	$tm_index = patterns_api_build_tagmodules_index();
	$format 	= $form_state['values']['format'];
	$filename = $form_state['values']['pattern_name'];
	// TODO: Loop here and create a modules section
	
	
	// Sets the category for the info
  // category: " . $modules_with_components[$form_state['values']['module']]
	$pattern = patterns_api_add_info_section($form_state['values']['info']);
	
	if (isset($form_state['values']['options']) && !empty($form_state['values']['options'])) {
		$cmp = $form_state['values']['options'];
		$tm_index = array($cmp => $tm_index[$cmp]);
		
		if (isset($form_state['values'][$cmp]) && !empty($form_state['values'][$cmp])) {
			$tag = $form_state['values'][$cmp];
			$func = $tm_index[$cmp][PATTERNS_EXPORT][$tag];
			$tm_index[$cmp][PATTERNS_EXPORT][PATTERNS_EXPORT_ALL] = $func;
		}
		//else {
		//	$tm_index[$cmp] = array($tag => $tm_index[$cmp][PATTERNS_EXPORT_ALL]);
		//}
		
	}
	
	$sections = patterns_extract_php($tm_index);
	foreach($sections as $s => $values) {
		$pattern[$s] = $values;
	} 
	//$pattern = array_merge($info, $sections);
	
	//if (patterns_validate_pattern($pattern, ))
	
	//$parsed = patterns_parser_parse($pattern, $format);
	drupal_set_message(print_r($pattern, true));
	
	if ($parsed) {
		drupal_set_message('Pattern succesfully extracted');
	}
	
	$result = patterns_io_save_pattern($pattern, $filename, $format);
	
	if ($result) {
		drupal_set_message('Pattern succesfully imported');
	}
	
	
	//patterns_extract_batch($tm_index);
	
	
	
	$form_state['redirect'] = 'admin/patterns/extract';
	/*
	// Only extract all for now
	
	$pattern = patterns_api_add_info_section();
	

	$exports = patterns_api_build_moduletags_index($form_state['values']['module'], null, PATTERNS_EXPORT);
	$func = $exports[$form_state['values']['options']];
	
	$sections = call_user_func_array($func, array());

	$pattern = array_merge($info, $sections);
	
	//debug($pattern);
	*/
}
function patterns_extract_php($tagmodules) {

	$result = array();
	foreach ($tagmodules as $tag => $values) {
		if (!isset($values[PATTERNS_EXPORT])) {
	  		continue;
	  }
		if (isset($values[PATTERNS_FILES])) {
			$files = (!is_array($values[PATTERNS_FILES])) ? array($values[PATTERNS_FILES]) : $values[PATTERNS_FILES];
    	foreach ($files as $file) {
      	require_once $file;
    	}
    }
    
	  $module = $values['module'];
	  $func = $values[PATTERNS_EXPORT][PATTERNS_EXPORT_ALL];
	  
		$result[$tag] = call_user_func($func);
    
  }

  return $result;
}

function patterns_extract_batch($tagmodules) {

	$cur_sec = 1;
	$tot_sec = count($tagmodules);
	foreach ($tagmodules as $tag => $values) {
		if (!isset($values[PATTERNS_EXPORT])) {
	  		continue;
	  }
		if (isset($values[PATTERNS_FILES])) {
			$files = (!is_array($values[PATTERNS_FILES])) ? array($values[PATTERNS_FILES]) : $values[PATTERNS_FILES];
    	foreach ($files as $file) {
      	require_once $file;
    	}
    }
	  $module = $values['module'];
	  $func = array($values[PATTERNS_EXPORT][PATTERNS_EXPORT_ALL]);
	  //foreach ($tags as $tag => $forms) {
		
	  	
		
	    $progress_params = array(
	                          '%section' => $module,
	                          '%cur_sec' => $cur_sec,
	                          '%tot_sec' => $tot_sec,
	    );
	
	    $progress = t('Running component "%section" (%cur_sec/%tot_sec), action @current out of @total', $progress_params);
			    
	    $batch = array(
	      //'title' => t('Processing section %section of pattern %pattern', array('%section' => $section, '%pattern' => $info['title'])),
	      'title' => t('Extracting configuration for %tag', array('%tag' => $tag)),
	      'progress_message' => $progress,
	      'finished' => 'patterns_extract_batch_finish',
	    	'operations' => array($func),
	      'init_message' => t('Initializing component "%section" (%cur_sec/%tot_sec)', array(
	                          '%section' => $module,
	                          '%cur_sec' => $cur_sec,
	                          '%tot_sec' => $tot_sec,
	                         )),
	    );
	
	    if (isset($values[PATTERNS_FILES]) && !empty($values[PATTERNS_FILES])) {
	   		//$batch['file'] = current($values[PATTERNS_FILES]); // should be an array or not?
	   		$batch['file'] = drupal_get_path('module','patterns_components') . 'components/taxonomy.inc';
	    }
	    
	                         
	      
	  //}
	
    $_SESSION['patterns_extract_batch_info'] = array('component' => $module, 'tag' => $tag);
    batch_set($batch);
    $cur_sec++;
  }

  return TRUE;
}

/**
 * Finishes a batch operation.
 * @TODO Doc.
 */
function patterns_extract_batch_finish($success, $results, $operations) {
  $info = $_SESSION['patterns_extract_batch_info'];
  $section = $results['module'];
  if (empty($results['abort'])) {
    foreach ($info as $key => $i) {
      drupal_set_message(t('Section "@section" of pattern "@pattern" ran successfully.', array('@pattern' => $i['title'],
                                                                                             '@section' => $section,
      )));
      $query_params = array(':time' => time(), // Note: time() != $_SERVER['REQUEST_TIME']
                            ':pid' => $key,
                            ':en' => PATTERNS_STATUS_ENABLED,
      );
      db_query("UPDATE {patterns} SET status = :en, enabled = :time WHERE pid = :pid", $query_params);
    }
  }
  else {
    $pattern = reset($info);
    drupal_set_message(t('Section "@section" of pattern "@pattern" ran with errors.
      Check the error messages to get more details.',
      array('@pattern' => $pattern['title'], '@section' => $section,
           )),
     'error');
    drupal_set_message($results['error_message'], 'error');
  }
  
  unset($_SESSION['patterns_batch_info']);
  drupal_flush_all_caches();
}


function patterns_extract_page2($form, &$form_state) {
	
	
	// TODO: Loop here and create a modules section
	
	
	// Sets the category for the info
  // category: " . $modules_with_components[$form_state['values']['module']]
	$pattern = patterns_api_add_info_section($form_state['values']['info']);
	
	$pattern = patterns_api_add_info_section();
	
	// Sets the category for the info
  // category: " . $modules_with_components[$form_state['values']['module']]

	$exports = patterns_api_build_moduletags_index($form_state['values']['module'], null, PATTERNS_EXPORT);
	$func = $exports[$form_state['values']['options']];
	
	$sections = call_user_func_array($func, array());

	$pattern = array_merge($info, $sections);
	
	//debug($pattern);
	
  return $form;
}


/**
 * validation function for import_module_form
 * checks in which step we are and puts the step counter up and sets the rebuild flag
 */
function patterns_extract_validate($form, &$form_state) {
	
	$step = _patterns_extract_check_step($form_state, TRUE);
	if ($step == 1) {
		return _patterns_extract_validate_step1($form, $form_state);
	}
	
  
	
  //$form_state['values']['step'] = $form_state['values']['step']+1;

  return TRUE;
}

function _patterns_extract_validate_step1($form, &$form_state) {
	
	if (!isset($form_state['values']['format'])){
		form_set_error('format', t('No valid format selected.'));
		return FALSE;
	}
	
	if (isset($form_state['values']['all']) && $form_state['values']['all'] == 0){
		return TRUE; // We extract all, no need for further checkings
	}
	
	/*
	if (!isset($form_state['values']['module'])){
		form_set_error('ext', t('No valid module selected.'));
		return FALSE;
	} 
	$module = $form_state['values']['module'];
	if (empty($module) || !patterns_api_is_valid_component_name($module)) {
		form_set_error('ext', t('No valid module selected.'));
		return FALSE;
	}
	*/
	
	if (!isset($form_state['values']['options'])){
		form_set_error($module, t('No valid component selected.'));
		return FALSE;
	}
	
	$component = $form_state['values']['options'];
	if (!isset($form_state['values'][$component])){
		form_set_error($module, t('No valid tag selected for component %component selected.', array('%component' => $component)));
		return FALSE;
	}
	
  return TRUE;
}