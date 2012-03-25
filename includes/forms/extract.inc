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
  	//if (isset($form_state['values']['module']) && !is_int($form_state['values']['module']))  {
			return patterns_extract_page2($form, $form_state);
  	//}
  }	

	// FORM SELECTED
  
  if ( $step == 3 && !isset($form_state['values']['content'])) {
  	return patterns_extract_page3($form, $form_state);
  }

  // show pattern import form
  if ($step >= 3) {
		return patterns_extract_page4($form, $form_state);
  }

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
	
//	if (isset($form_state['values'][PATTERNS_EXTRACT_ALL])) {
//    $step = 4;
//  }
	if (isset($form_state['values']['content'])) {
    $step = 4;
  }
	elseif (isset($form_state['values']['module_forms'])) {
    $step = 3;
  }
	elseif (isset($form_state['values']['module'])) {
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
 	
	
  if ($step < 1 || $step > 5) {
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

	/*
	// Not for now. To move into the API
	 	 
	if (!patterns_utils_is_macro_enabled()) {
  	drupal_set_message(t('You need to enable the macro module for the import from module function. You can get it <a href="@url">here</a>.',
      array('@url' => url('http://drupal.org/project/macro/'))), 'error');
    return;
  }
  
	*/
	
 	
 	$step = _patterns_extract_check_step($form_state);
	_patterns_extract_if_invalid_step_goback($step);
	$form['step'] = array(
     '#type' => 'hidden',
     '#title' => t('Pattern Identifier'),
     '#description' => t('step'),
     '#value' => $step,
  );
	
  //debug($step);
  
  drupal_add_js('misc/ajax.js');
  $name = isset($form_state['values']['name']) ?  $form_state['values']['name'] : 'name not set';
  
  patterns_io_load_components();

  return _patterns_extract_goto($form, $form_state, $step);

}

function _patterns_extract_add_ajax_to_options (&$form) {
	if (!isset($form['options'])) return FALSE;
	if (!isset($form['module'])) return FALSE;
	
	$module = $form['module'];
	$idx_module = $module . '_forms-wrapper';
	$form['module_forms_container'] = array(
    '#type' => 'fieldset',
    '#prefix' => '<div id="' . $idx_module . '">',
    '#suffix' => '</div>',
  );
	
	$form['options'] += array(
    '#ajax' => array(
     'wrapper' => $idx_module,
     'callback' => 'patterns_import_module_ajax_module_forms_callback',
    ),
  ); 
	  
/*
   
   $form['module_forms_container']['checkall'] = array(
  	'#type' => 'checkboxes',
    '#attributes' => array('onclick' => 'checkUncheckAll(this);'),
    '#options' => array('UnCheckAll' => "Select / Unselect all"),
  );
   
   // javascript for the check / uncheck all function
  drupal_add_js("function checkUncheckAll(theElement) {
        var theForm = theElement.form, z = 0;
        for(z=0; z<theForm.length;z++){
          if (theForm[z].type == 'checkbox' && theForm[z].name != 'checkall'){
            theForm[z].checked = theElement.checked;
          }
        }
      }
    ", 'inline');

*/

	 return TRUE;
}


function patterns_extract_page1($form, &$form_state) {
	 
	$tm_index = patterns_api_build_tagmodules_index(null, null, PATTERNS_EXPORT);
	
  $form = array ();
   	
  $form['ext'] = array (
		'#type' => 'fieldset',
	  '#title' => 'Components with automatic extraction enabled',
	);
	
  foreach ($tm_index as $module => $tags) {

  	/*
    $form['ext'][$module] = array(
    	'#type' => 'fieldset',
	    '#title' => $module,
  		'#collapsible' => TRUE,
  		'#collapsed' => FALSE, // TODO set to TRUE after debugging
    );
    
       // show all forms as checkboxes
   $form['ext'][$module]['module_forms'] = array(
      '#type' => 'checkboxes',
      '#multiple' => TRUE,
      '#options' => array_keys($tags),
      '#size' => count($options, COUNT_RECURSIVE),
      //'#title' => $module,
      //'#default_value' => isset($form_state['values']['module_forms']) ? $form_state['values']['module_forms'] : array(),
      //'#disabled' => $step < 3 ? FALSE : TRUE,
   );

   
    
   foreach ($tags as $tag => $forms) {
	   $form['ext'][$module]['module_forms'][$tag] = array(
	      '#type' => 'checkboxes',
	      '#multiple' => TRUE,
	      '#options' => $forms,
	      //'#size' => count($options, COUNT_RECURSIVE),
	      '#title' => $tag,
	      //'#default_value' => isset($form_state['values']['module_forms']) ? $form_state['values']['module_forms'] : array(),
	      //'#disabled' => $step < 3 ? FALSE : TRUE,
	   );
   }
	 
  }
 */

  
  
  
  	$form['ext'][$module] = array (
	    '#type' => 'fieldset',
	    '#title' => $module,
  		'#collapsible' => TRUE,
  		'#collapsed' => FALSE, // TODO set to TRUE after debugging
	  );
	  
	  $form['ext'][$module]['module'] = array(
	    '#type' => 'hidden',
	  	'#default_value' => $module,
	  ); 
	  
	  
	  $form['ext'][$module]['options'] = array(
	    '#type' => 'radios',
	    '#title' => t('Extraction options'),
	    '#options' => array_combine(array_keys($tags), array_keys($tags)),
	    //'#disabled' => $step < 2 ? FALSE : TRUE, // disable when chosen
	  ); 
	 
	  
	  
	  foreach ($tags as $tag => $forms) {
			/*
	  	$form['ext'][$module][$tag] = array(
	    	'#type' => 'fieldset',
	    	'#title' => $tag,
	  		//'#collapsible' => TRUE,
  			//'#collapsed' => FALSE, // TODO set to TRUE after debugging
	    	//'#options' => array_combine(array_keys($tags), array_keys($tags)),
	    	//'#disabled' => $step < 2 ? FALSE : TRUE, // disable when chosen
	 	 	);
	  */
	  	$form['ext'][$module][$tag]['tag'] = array(
	    	'#type' => 'hidden',
	  		'#default_value' => $tag,
	  	); 
	  	
	  	$form['ext'][$module][$tag]['options'][$tag] = array(
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
	  
	  
	  $form['ext']['module'] = array(
	    '#type' => 'hidden',
	  	'#default_value' => $module,
	  ); 
	  
  }
  
  
  $form = patterns_forms_get_formats_selector($form,
  																						patterns_forms_get_default_format(),
  																						'Extract data in this format',
  																						FALSE
  );
  
  $form['submit'] = array(
  	'#type' => 'submit',
    '#value' => t('Next Step'),
  );
  $form['#validate'][] = 'patterns_extract_validate';
  
  return $form;
}

function patterns_extract_page2_old($form, &$form_state) {
	   // now get the forms and save them into $actions
   $module_name = $form_state['values']['module'];
   
   // first check if user might want to import from all
   if ($module_name != $import_from_all_string) {
    $actions = call_user_func($module_name . "_patterns", NULL);
   }
   else {
    $actions = array();
    foreach ($modules_with_components as $module) {
     if ($module == $import_from_all_string) continue;
     $actions = array_merge($actions, call_user_func($module . "_patterns", NULL));
    }

   }

   // get all the forms
   $module_form_ids = array();
   $files = array();
   foreach ($actions as $entry) {
    $files = array_merge(isset($entry[PATTERNS_FILES]) ?  $entry[PATTERNS_FILES] : array(), $files);
   }

   // build array of forms for showing as checkboxes
   // current output per form: <tag> : <descr> '<action>' : <myform>
   $options = array();
   $counter = 0;
   foreach ($actions as $tag => $entry) {
    $descr = $entry[PATTERNS_INFO];
    foreach ($entry as $action => $forms) {
     if ($action != PATTERNS_INFO && $action != PATTERNS_FILES && $action != PATTERNS_CREATE && $action != PATTERNS_DELETE) {
      foreach ($forms as $my_form) {
       $key = "o" . $counter;
       $options = array_merge($options, array($key => $tag . ": " . $descr . " '" . $action . "': " . $my_form));
       $counter++;
      }
     }
    }
   }

   // show all forms as checkboxes
   $form['module_forms_container']['module_forms'] = array(
      '#type' => 'checkboxes',
      '#multiple' => TRUE,
      '#options' => $options,
      '#size' => count($options, COUNT_RECURSIVE),
      '#title' => t('Which forms should be inspected?'),
      '#default_value' => isset($form_state['values']['module_forms']) ? $form_state['values']['module_forms'] : array(),
      //'#disabled' => $step < 3 ? FALSE : TRUE,
   );
   
   $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Import')
   );
   $form['#validate'][] = 'patterns_extract_validate';
   $form['#submit'][] = 'patterns_import_submit';
   
     // Not for now
  
   return $form;
}

function patterns_extract_all($form, &$form_state) {
	

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


function patterns_extract_page2($form, &$form_state) {
	

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

function patterns_extract_page4($form, &$form_state) {
	
   $def_dir = patterns_path_get_files_dir();

   if (!isset($yaml))
   $yaml = $form_state['values']['content'];
   // patterns_editor('patterns_import_source', $yaml);
   $validation = t('Pattern hasn\'t been validated yet.');
   $form['validation_result'] = array(
      '#markup' => '<br><strong>' . $validation . '</strong>',
      '#prefix' => '<div id="validation_result">',
      '#suffix' => '</div>',
   );
   $form['name'] = array(
       '#type' => 'textfield',
       '#title' => t('Pattern Identifier'),
       '#description' => t('Machine readable name for the pattern. The actual title should be included in the pattern itself.'),
       '#required' => TRUE,
   );

   $form['content'] = array(
       '#type' => 'textarea',
       '#rows' => 25,
       '#title' => t('Enter Pattern Source Code'),
       '#default_value' => $yaml,
       '#description' => t('Imported patterns are not executed until you run them manually.'),
   );

   $form['validate'] = array(
       '#prefix' => '<span style=\'margin-right:10px\'>',
       '#markup' => "<a href='#' id='validate_pattern'>" . t('Validate') . "</a>",
       '#suffix' => '</span> ',
   );

   // TODO: transform in a FILE form
   $form['destination'] = array(
       '#type' => 'textfield',
       '#title' => t('Destination'),
       '#description' => t('Pattern will be saved in this directory. Must be writable. Default dir: @dir', array('@dir' => $def_dir)),
       '#default_value' => $def_dir,
       '#required' => TRUE,
   );
   if (!patterns_load_co2()) {
    $co2 = l(t('Codemirror'), 'http://codemirror.net/');
    $editor .= t('Install !co2 in sites/all/libraries/codemirror2 for advanced editing functionalities.', array('!co2' => $co2));
   }

   // Load the JS of for the ajax validation.
   drupal_add_js(drupal_get_path('module', 'patterns') . '/js/ajax_validation.js');
   
   return $form;
}

/**
 * Implementation of hook_form_alter().
 * needed for deactivating actual submission of the forms when submitting, i.e.
 * we only want to know what gets submitted without the submission getting
 * processed
 */
function patterns_form_alter(&$form, $form_state, $form_id) {
  // Add the record callback on submit and delete all others
  if ($form_id != 'macro_import_macro' && variable_get('macro_enabled', FALSE) && variable_get('patterns_import_module', FALSE)) {
   $form['#submit'] = array();
   $form['#submit'][] = 'macro_record_macro';
  }
}

/**
 * in case error happens and script gets halted
 */
function patterns_import_module_error_handler($errno, $errstr) {
  variable_set('macro_enabled', FALSE);
  variable_set('macro_submissions', array());
  variable_set('patterns_import_module', FALSE);
  restore_error_handler();
  // normal error handler should take over from here
  return FALSE;
}

/**
 * validation function for import_module_form
 * checks in which step we are and puts the step counter up and sets the rebuild flag
 */
function patterns_extract_validate($form, &$form_state) {
	
	$form_state['rebuild'] = TRUE;
	$step = _patterns_extract_check_step($form_state, TRUE);
	if ($step == 1) {
		return _patterns_extract_validate_step1($form, $form_state);
	}
	
  
  //$form_state['values']['step'] = $form_state['values']['step']+1;

  return TRUE;
}

function _patterns_extract_validate_step1($form, &$form_state) {
	
	if (!isset($form_state['values']['module'])){
		form_set_error('ext', t('No valid module selected.'));
		return FALSE;
	} 
	$module = $form_state['values']['module'];
	if (empty($module) || !patterns_api_is_valid_component_name($module)) {
		form_set_error('ext', t('No valid module selected.'));
		return FALSE;
	}
	
	if (!isset($form_state['values']['options'])){
		form_set_error($module, t('No valid option selected for module %module selected.', array('%module' => $module)));
		return FALSE;
	}
	
	if (!isset($form_state['values']['format'])){
		form_set_error('format', t('No valid format selected.'));
		return FALSE;
	}
	
  return FALSE;
}

function patterns_extract_visualize_step_process($step) {

	// TODO: check if we need it.
	
	$conn = '->';
	$step1 = t('Select a module');
	$step2 = t('Select a module');
	
	$form = array();
	$form['step_process'] = array(
	  '#type' => 'fieldset',
	  '#title' => 'Step flow',
	);
	
	$form['step_process']['step'] = array (
		'#markup' => $txt,
	);
	
  return FALSE;
}

function patterns_import_module_ajax_module_forms_callback($form, $form_state) {
  return $form['module_forms_container'];
}


/**
 * Forms to create a yaml file based on a module
 * longer description:
 * Displays forms to choose a module (which implements the pattern hooks) and
 * the forms which we get from module_patterns,
 * then submits the forms and captures the form submissions with the Macro module.
 * From this, yaml file gets generated
 */

function patterns_extract_old($form, &$form_state) {

  $list = module_list();
  if (!isset($list["macro"])) {
    drupal_set_message(t('You need to enable the macro module for the import from module function. You can get it <a href="@url">here</a>.',
      array('@url' => url('http://drupal.org/project/macro/'))), 'error');
    return;
  }

 drupal_add_js('misc/ajax.js');
  // check if 'step' is undefined and show current step
  if (!isset($form_state['values']['step']))
    $form_state['values']['step'] = 1;
  if (isset($form_state['values']['module']))
    $form_state['values']['step'] = 2;
  if (isset($form_state['values']['module_forms']))
    $form_state['values']['step'] = 3;
  if (isset($form_state['values']['content']))
    $form_state['values']['step'] = 4;

  $step = $form_state['values']['step'];
  $name = isset($form_state['values']['name']) ?  $form_state['values']['name'] : 'name not set';
  $form['step'] = array(
     '#type' => 'hidden',
     '#title' => t('Pattern Identifier'),
     '#description' => t('step'),
     '#value' => $step,
  );

  // for all modules, check if hook_patterns() is defined and let the user choose
  // a module from a radio button form
  $enabled_modules = module_list();
  $modules_with_components = array();
  // the string for the 'import from all' checkbox
  $import_from_all_string = t('Import from every module');
  array_push($modules_with_components, $import_from_all_string);

  // need to load components to be able to check
  // TODO: refactor: this does something very similar to patterns_get_tagmodules_index
  patterns_io_load_components();
  foreach ( module_implements('patterns') as $module)
    array_push($modules_with_components, $module);

  // show all modules that implement the hook
  $form['module'] = array(
    '#type' => 'radios',
    '#title' => t('Choose a module to extract the pattern'),
    '#options' => $modules_with_components,
    '#default_value' => isset($form_state['values']['module']) ? $form_state['values']['module'] : '0',
    '#ajax' => array(
      'wrapper' => 'module_forms-wrapper',
      'callback' => 'patterns_import_module_ajax_module_forms_callback',
    ),
    //'#disabled' => $step < 2 ? FALSE : TRUE, // disable when chosen
  );

  $form['module_forms_container'] = array(
    '#type' => 'fieldset',
    // These provide the wrapper referred to in #ajax['wrapper'] above.
    '#prefix' => '<div id="module_forms-wrapper">',
    '#suffix' => '</div>',
  );
   // javascript for the check / uncheck all function
  drupal_add_js("function checkUncheckAll(theElement) {
        var theForm = theElement.form, z = 0;
        for(z=0; z<theForm.length;z++){
          if (theForm[z].type == 'checkbox' && theForm[z].name != 'checkall'){
            theForm[z].checked = theElement.checked;
          }
        }
      }
    ", 'inline');

  $form['module_forms_container']['checkall'] = array(
      '#type' => 'checkboxes',
      '#attributes' => array('onclick' => 'checkUncheckAll(this);'),
      '#options' => array('UnCheckAll' => "Select / Unselect all"),
  );
  if (isset($form_state['values']['module']) && !is_int($form_state['values']['module']))  {
   // now get the forms and save them into $actions
   $module_name = $modules_with_components[$form_state['values']['module']];
   // first check if user might want to import from all
   if ($module_name != $import_from_all_string) {
    $actions = call_user_func($module_name . "_patterns", NULL);
   }
   else {
    $actions = array();
    foreach ($modules_with_components as $module) {
     if ($module == $import_from_all_string) continue;
     $actions = array_merge($actions, call_user_func($module . "_patterns", NULL));
    }

   }

   // get all the forms
   $module_form_ids = array();
   $files = array();
   foreach ($actions as $entry) {
    $files = array_merge(isset($entry[PATTERNS_FILES]) ?  $entry[PATTERNS_FILES] : array(), $files);
   }

   // build array of forms for showing as checkboxes
   // current output per form: <tag> : <descr> '<action>' : <myform>
   $options = array();
   $counter = 0;
   foreach ($actions as $tag => $entry) {
    $descr = $entry[PATTERNS_INFO];
    foreach ($entry as $action => $forms) {
     if ($action != PATTERNS_INFO && $action != PATTERNS_FILES && $action != PATTERNS_CREATE && $action != PATTERNS_DELETE) {
      foreach ($forms as $my_form) {
       $key = "o" . $counter;
       $options = array_merge($options, array($key => $tag . ": " . $descr . " '" . $action . "': " . $my_form));
       $counter++;
      }
     }
    }
   }

   // show all forms as checkboxes
   $form['module_forms_container']['module_forms'] = array(
      '#type' => 'checkboxes',
      '#multiple' => TRUE,
      '#options' => $options,
      '#size' => count($options, COUNT_RECURSIVE),
      '#title' => t('Which forms should be inspected?'),
      '#default_value' => isset($form_state['values']['module_forms']) ? $form_state['values']['module_forms'] : array(),
      //'#disabled' => $step < 3 ? FALSE : TRUE,
   );

  }


  if ( $step == 3 && !isset($form_state['values']['content'])) {
   // get macros of chosen forms
   $yaml_content = array();
   $yaml_content[PATTERNS_CREATE] = '';
   $yaml_content[PATTERNS_MODIFY] = '';
   $yaml_content[PATTERNS_DELETE] = '';

   variable_set('patterns_import_module', TRUE);
   variable_set('macro_enabled', TRUE);
   variable_set('macro_submissions', array());
   $chosen_forms = $form_state['values']['module_forms'];

   // same loop as above for creating the checkboxes

   // for measuring the perfomance / time, k-best
   $measure = FALSE;
   if ($measure) {
    $runs = 1000;
    $k = 3;
    $start = microtime(TRUE);
    $j = 0;
    $min = 10000000;
   }
   else {
    $runs = 1;
    $k = 1;
    $j = 0;
   }


   $i=0;
   while ($j<$k) {
    $j = $j+1;
    while ($i<$runs) {
     $i = $i+1;


     $counter = 0;
     foreach ($actions as $tag => $entry) {
      $descr = $entry[PATTERNS_INFO];
      foreach ($entry as $action => $forms) {
       if ($action != PATTERNS_INFO && $action != PATTERNS_FILES && $action != PATTERNS_CREATE && $action != PATTERNS_DELETE) {
        foreach ($forms as $my_form) {
         //
         //$key = ;
         if ($chosen_forms["o" . $counter]) {
          // include needed files
          if (isset($entry[PATTERNS_FILES])) {
           foreach ($entry[PATTERNS_FILES] as $file) {
            require_once $file;
           }
          }
          // check if we get argument for the form and if we get them for
          // several runs, i.e. for extracting all data
          $loop = FALSE;
          if (function_exists($module_name . "_patterns_get_arguments")) {
            set_error_handler("patterns_import_module_error_handler", E_ERROR | E_CORE_ERROR | E_COMPILE_ERROR | E_USER_ERROR);
            $arguments = call_user_func_array($module_name . "_patterns_get_arguments", array($action, $tag, $my_form, &$loop));
          }
          else {
           $arguments = array("");
          }

          // small transformation for the loop below:
          // arguments for one run must be put in a array
          if (!$loop) {
           $arguments = array($arguments);
          }

          // go through all argumentarrays for all the runs
          foreach ($arguments as $arguments_element) {
           // submit and save output
           $module_form_state = array();
           $module_form_state['values'] = array();
           variable_set('macro_submissions', array());

           $module_form_state['build_info']['args'] = $arguments_element;
           drupal_form_submit($my_form, $module_form_state);

           $my_macro = macro_get_macro_yaml();
           // format output: trim --- and whitespaces at the beginning and end
           // and then indent correctly
           $my_macro = substr($my_macro, 4, strlen($my_macro)-8);
           $my_macro = "      " . str_replace("\n", "\n      ", $my_macro);
           $clean_tag = str_replace("_extract_all" , "", $tag);
           $clean_tag = str_replace("_extract", "", $clean_tag);
           $yaml_content[$action] .= "  - " . $action . ":\n" .
                                            "      tag: " . $clean_tag . "\n" .
           $my_macro . "\n\n";
          }
         }
         $counter++;
        }
       }
      }
     }

    }

    if ($measure) {
     $time = (microtime(TRUE) - $start);
     $min = $time < $min ? $time : $min;
     // TODO: why filter_xss?
     drupal_set_message(filter_xss("testing with " . $runs . " runs, k = " . $j . ", time is: " . $time));
    }
   }
   if ($measure)
     drupal_set_message(filter_xss("k-best: " . $min)); // TODO: why filter_xss?
   variable_set('macro_enabled', FALSE);
   variable_set('patterns_import_module', FALSE);

   // creating info section of yaml pattern file
   $yaml = "# YAML " . $modules_with_components[$form_state['values']['module']] . " Pattern\n" .
          "\n" .
          "info:\n" .
          "  title: #please add a title\n" .
          "  description: " .
          "automatically generated Pattern file";

   global $user;
   global $base_root;
   // TODO: get core version
   $yaml .= "\n" .
          "  author: " . $user->name . "\n" .
          "  category: " . $modules_with_components[$form_state['values']['module']] . "\n" .
          "  version: 1.0\n" .
          "  core: x.y\n" .
          "  author_email: " . $user->mail . "\n" .
          "  author_website: " . $base_root . "\n" .
          "\n" .
          "actions:\n" .
          "";
   // concatenating the actions
   $yaml .= $yaml_content[PATTERNS_CREATE];
   $yaml .= $yaml_content[PATTERNS_MODIFY];
   $yaml .= $yaml_content[PATTERNS_DELETE];
  }

  // show pattern import form
  if ($step >= 3) {
   $def_dir = patterns_path_get_files_dir();

   if (!isset($yaml))
   $yaml = $form_state['values']['content'];
   // patterns_editor('patterns_import_source', $yaml);
   $validation = t('Pattern hasn\'t been validated yet.');
   $form['validation_result'] = array(
      '#markup' => '<br><strong>' . $validation . '</strong>',
      '#prefix' => '<div id="validation_result">',
      '#suffix' => '</div>',
   );
   $form['name'] = array(
       '#type' => 'textfield',
       '#title' => t('Pattern Identifier'),
       '#description' => t('Machine readable name for the pattern. The actual title should be included in the pattern itself.'),
       '#required' => TRUE,
   );

   $form['content'] = array(
       '#type' => 'textarea',
       '#rows' => 25,
       '#title' => t('Enter Pattern Source Code'),
       '#default_value' => $yaml,
       '#description' => t('Imported patterns are not executed until you run them manually.'),
   );

   $form['validate'] = array(
       '#prefix' => '<span style=\'margin-right:10px\'>',
       '#markup' => "<a href='#' id='validate_pattern'>" . t('Validate') . "</a>",
       '#suffix' => '</span> ',
   );

   // TODO: transform in a FILE form
   $form['destination'] = array(
       '#type' => 'textfield',
       '#title' => t('Destination'),
       '#description' => t('Pattern will be saved in this directory. Must be writable. Default dir: @dir', array('@dir' => $def_dir)),
       '#default_value' => $def_dir,
       '#required' => TRUE,
   );
   if (!patterns_load_co2()) {
    $co2 = l(t('Codemirror'), 'http://codemirror.net/');
    $editor .= t('Install !co2 in sites/all/libraries/codemirror2 for advanced editing functionalities.', array('!co2' => $co2));
   }

   // Load the JS of for the ajax validation.
   drupal_add_js(drupal_get_path('module', 'patterns') . '/js/ajax_validation.js');
  }

  // show different submit buttons depending on steps
  if ($step <= 2) {
   $form['submit'] = array(
     '#type' => 'submit',
     '#value' => t('Next Step'),
   );
   $form['#validate'][] = 'patterns_import_module_validate';
   $form['#submit'][] = 'patterns_import_module_submit';
  }
  elseif ($step > 2)  { // form elements needed to import yaml file
   $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Import')
   );
   $form['#validate'][] = 'patterns_import_validate';
   $form['#submit'][] = 'patterns_import_submit';
  }

  return $form;
}