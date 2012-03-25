<?php
/**
 * @file
 * Importing Patterns from a file or using an URL.
 */

/**
 * Forms to create a yaml file based on a module
 * longer description:
 * Displays forms to choose a module (which implements the pattern hooks) and
 * the forms which we get from module_patterns,
 * then submits the forms and captures the form submissions with the Macro module.
 * From this, yaml file gets generated
 */

function patterns_import_module($form, &$form_state) {

	if (!patterns_utils_is_macro_enabled()) {
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
  //foreach ( module_implements('patterns') as $module)
  //  array_push($modules_with_components, $module);

  $tm_index = patterns_api_build_tagmodules_index(null, PATTERNS_EXPORT);
  //$modules_with_components = array_keys($tm_index);
  
  $form = array();
  foreach ($tm_index as $module => $options) {
  	
  	$form[$module] = array (
	    '#type' => 'fieldset',
	    '#title' => $module,
  		'#collapsible' => TRUE,
  		'#collapsed' => TRUE,
	  );
	  $form[$module]['options'] = array(
	    '#type' => 'radios',
	    '#title' => t('Extraction options'),
	    '#options' => array_keys($options),
	    //'#default_value' => isset($form_state['values']['module']) ? $form_state['values']['module'] : '0',
	    '#ajax' => array(
	      'wrapper' => 'module_forms-wrapper',
	      'callback' => 'patterns_import_module_ajax_module_forms_callback',
	    ),
	    //'#disabled' => $step < 2 ? FALSE : TRUE, // disable when chosen
	  );
	  
	  
  }
  
  // GOES INTO A FORM
  
  // Not for now
  
	/*
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
  */
  
  // END FORM
  
  
  // MODULE SELECTED
  
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

	// FORM SELECTED
  
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
     
           $my_macro = patterns_api_extract($my_form, $arguments_element);

           
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
function patterns_import_module_validate($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  //$form_state['values']['step'] = $form_state['values']['step']+1;

  return TRUE;
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

function patterns_import_module_old($form, &$form_state) {

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