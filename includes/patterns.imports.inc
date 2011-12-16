<?php
/**
 * @file
 * Importing Patterns from a file or using an URL.
 */

/**
 * Form constructor for the Pattern import form.
 *
 * @see patterns_import_validate()
 * @see patterns_import_submit()
 * @ingroup forms
 */
function patterns_import_source($form, &$form_state) {

  $def_dir = patterns_config_get_files_dir();
  
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
    '#description' => t('Imported patterns are not executed until you run them manually.'),
  );
  
  // TODO: transform in a FILE form
  $form['destination'] = array(
    '#type' => 'textfield',
    '#title' => t('Destination'),
    '#description' => t('Pattern will be saved in this directory. Must be writable. Default dir: @dir', array('@dir' => $def_dir)),
    '#default_value' => $def_dir,
    '#required' => TRUE,
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  // ? Double array ?
  $form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}

/**
 * Display the import pattern file form
 */
function patterns_import_file($form, &$form_state) {

  $def_dir = patterns_config_get_files_dir();
  
  $form['#attributes']['enctype'] = 'multipart/form-data';

  $form['pattern_uri'] = array(
    '#type' => 'file',
    '#title' => t('Upload Pattern File'),
    '#description' => t('Imported patterns are not executed until you run them manually.'),
    '#size' => 48
  );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Pattern Identifier'),
    '#description' => t('Machine readable name for the pattern. The actual title should be included in the pattern itself.'),
    '#required' => TRUE
  );
  
  // TODO: transform in a FILE form
  $form['destination'] = array(
    '#type' => 'textfield',
    '#title' => t('Destination'),
    '#description' => t('Pattern will be saved in this directory. Must be writable. Default dir: @dir', array('@dir' => $def_dir)),
    '#default_value' => $def_dir,
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  //$form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}

/**
 * Display the import pattern url form
 */
function patterns_import_url($form, &$form_state) {
  
  $def_dir = patterns_config_get_files_dir();

  $form['pattern_uri'] = array(
    '#type' => 'textfield',
    '#title' => t('Specify an URI'),
    '#description' => t('Import a pattern from a remote URL. Imported patterns are not executed until you run them manually.'),
    '#size' => 48
  );

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Pattern Identifier'),
    '#description' => t('Machine readable name for the pattern. The actual title should be included in the pattern itself.'),
    '#required' => TRUE
  );
  
  $form['destination'] = array(
    '#type' => 'textfield',
    '#title' => t('Destination'),
    '#description' => t('Pattern will be saved in this directory. Must be writable. Default dir: @dir', array('@dir' => $def_dir)),
    '#default_value' => $def_dir,
    '#required' => TRUE,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Import')
  );

  // Now it has his own
  $form['#validate'][] = 'patterns_import_validate';
  $form['#submit'][] = 'patterns_import_submit';

  return $form;
}


/**
 * Validate handler for file importing
 */
function patterns_import_file_validate($form, &$form_state) {
  
  $destination = $form_state['values']['destination'];
  
  // If not automatically returns
  _patterns_imports_check_dirs($destination);
  _patterns_imports_check_name($form_state['values']['name']);

  $uri = $form_state['values']['pattern_uri'];

  // arg must be a string with a space separated list of allowed extensions.
  $validators = array('file_validate_extensions' => array('xml yaml'));
  
  if ($file = file_save_upload('pattern_uri', $validators)) {
    $form_state['values']['content'] = file_get_contents($file->uri); // was $file->filepath
    $form_state['pattern_uri'] = $file;
    $form_state['pattern_uri']->destination = $destination;
  }
  else {
   
    form_set_error('pattern_uri', t('Failed to retreive the pattern specified: %uri. Check your file and try again. Notice: only yaml and xml files are accepted', array('%uri' => $uri)));
    return FALSE;
  }

  $format = pathinfo($form_state['pattern_uri']->uri, PATHINFO_EXTENSION);

  // TODO: PHP format
  if ($format == 'yaml') {
    // YAML
    $patternscan = _patterns_scan_pattern($form_state['values']['content']);
    if (!_patterns_validate_patternscan($patternscan)) {
      $errors_on_scan = _patterns_analyze_patternscan($patternscan);
      form_set_error('name', t('Error parsing the YAML file: ' . implode(', ', $errors_on_scan)));
    }
  }
  else { // XML or other
    if (!patterns_parser_validate_xml_pattern($form_state['values']['content'])) {
      form_set_error('xmlsource', t('Error parsing the XML, please check your syntax and try again.'));
    }
  }

  // TODO: Check what happens with different formats.
  $form_state['values']['format'] = $format;

  return TRUE;
}

function patterns_import_validate($form, &$form_state) {

  $format = 'yaml';

  $destination = $form_state['values']['destination'];
  
  // If not automatically returns
  _patterns_imports_check_dirs($destination);
  _patterns_imports_check_name($form_state['values']['name']);

  if (!isset($form_state['values']['content'])) { // It is not an import_source
    $uri = $form_state['values']['pattern_uri'];

    if (empty($uri)) {
      form_set_error('pattern_uri', t('Invalid file or url provided.'));
      return FALSE;
    }

    $format = pathinfo($uri, PATHINFO_EXTENSION);
    $validators = array(); // filter out the extension

    // Remote File
    if (stripos($uri, 'http://') == 1) {
      if (!ini_get('allow_url_fopen')) {
        form_set_error('url', t('allow_url_fopen must be enabled in your php configuration in order to use this feature.'));
        return FALSE;
      }
      // Only YAML and XML extension accepted for remote files
      $validators = array('file_validate_extensions' => array('xml', 'yaml'));
    }
    
    if ($content = file_get_contents($uri)) {
      $form_state['values']['content'] = $content;
    }
    else {
      form_set_error('pattern_uri', t('Failed to retreive the pattern specified: %uri. Check your file and try again. Notice: only yaml and xml files are accepted', array('%uri' => $uri)));
      return FALSE;
    }
  }

//  // Name Validation
//  $name = $form_state['values']['name'];
//  $errors_on_name = _patterns_parser_analyze_pattern_name($name);
//  if (!empty($errors_on_name)){
//    form_set_error('name', t('Errors on the name of the pattern: ' . implode(', ', $errors_on_name)));
//    return FALSE;
//  }
//
//  if ($obj = patterns_db_load_pattern_from_name($name)) {
//    form_set_error('name', t('A pattern file with the same name alrady exists: !name', array('!name' => $name)));
//    return FALSE;
//  }

  // FORMAT Validation
  if ($format == 'yaml') {
    // YAML
    $patternscan = _patterns_scan_pattern($form_state['values']['content']);
    if (!_patterns_validate_patternscan($patternscan)) {
      $errors_on_scan = _patterns_analyze_patternscan($patternscan);
      form_set_error('name', t('Error parsing the YAML file: ' . implode(', ', $errors_on_scan)));
    }
  }
  else { // XML or other
    if (!patterns_parser_validate_xml_pattern($form_state['values']['content'])) {
      form_set_error('xmlsource', t('Error parsing the XML, please check your syntax and try again.'));
    }
  }

  // TODO: Check what happens with different formats
  $form_state['values']['format'] = $format;

  return TRUE;

}

function patterns_import_submit($form, &$form_state) {
  
  $success = FALSE;
  $name = $form_state['values']['name'];

  if (!empty($name)) {
    $content = $form_state['values']['content'];
    $format = (isset($form_state['values']['format'])) ? $form_state['values']['format'] : 'yaml';


    // What is this?
//    if (isset($form_state['pattern_file'])) {
//      $success = file_copy($form_state['pattern_file']->filepath, $form_state['pattern_file']->destination);
//    }
    //else
    if (!empty($content)) {
      $success = patterns_io_save_pattern($content, $name, $format, $form_state['values']['destination']);
    }
  }

  if ($success) {
    drupal_set_message(t('Pattern successfully imported.'));
  }
  else {
    drupal_set_message(t('File couldn\'t be saved on the server. Import failed.'), 'error');
  }
  $form_state['redirect'] = 'admin/patterns';
}


/**
 * Forms to create a yaml file based on a module
 * longer description:
 * Displays forms to choose a module (which implements the pattern hooks) and 
 * the forms which we get from module_patterns,
 * then submits the forms and captures the form submissions with the Macro module.
 * From this, yaml file gets generated
 */

function patterns_import_module($form, &$form_state) {
 // check if 'step' is undefined and show current step
 if(!isset($form_state['values']['step']))
   $form_state['values']['step'] = 1;
   $step = $form_state['values']['step'];
   $name = isset($form_state['values']['name']) ?  $form_state['values']['name'] : 'name not set';
   $form['step'] = array(
    '#type' => 'textfield',
    '#title' => t('Pattern Identifier'),
    '#description' => t('step'),
    '#value' => t($step),
    '#disabled' => TRUE,
 );

 // for all modules, check if hook_patterns() is defined and let the user choose
 // a module from a radio button form
 $enabled_modules = module_list(); 
 $modules_with_components = array();
 // need to load components to be able to check
 patterns_load_components();
 foreach ( module_implements('patterns') as $module)
    array_push($modules_with_components, $module);
 
 // show all modules that implement the hook
 $form['module'] = array(
   '#type' => 'radios',
   '#title' => t('Choose a module to extract the pattern'),
   //'#default_value' => -1,
   '#options' => $modules_with_components,
   '#default_value' => isset($form_state['values']['module']) ? $form_state['values']['module'] : '0',
   '#disabled' => $step < 2 ? FALSE : TRUE, // disable when chosen
 );
 
 if($step > 1)  {
    // now get the forms and save them into $actions
    $module_name = $modules_with_components[$form_state['values']['module']];
    $actions = call_user_func($module_name . "_patterns");
    
    // get all the forms
    $module_form_ids = array();
    $files = array();
    foreach($actions as $entry) {
        $files = array_merge(isset($entry[PATTERNS_FILES]) ?  $entry[PATTERNS_FILES] : array(), $files);
    }
    
    // build array of forms for showing as checkboxes
    // current output per form: <tag> : <descr> '<action>' : <myform>
    $options = array();
    $counter = 0;
    foreach($actions as $tag => $entry) {
      $descr = $entry[PATTERNS_INFO];
      foreach($entry as $action => $forms) { 
        if($action != PATTERNS_INFO && $action != PATTERNS_FILES && $action != PATTERNS_CREATE && $action != PATTERNS_DELETE) {
          foreach($forms as $my_form) {
            $key = "o".$counter;
            $options = array_merge($options, array($key => $tag . ": " . $descr . " '" . $action . "': " . $my_form));
            $counter++;
          }
        } 
      }
    }

    // show all forms as checkboxes
    $form['module_forms'] = array(
     '#type' => 'checkboxes',
     '#multiple' => TRUE,
     '#options' => $options,      
     '#size' => count($options, COUNT_RECURSIVE), 
     '#title' => t('Which forms should be inspected?'),
     '#default_value' => isset($form_state['values']['module_forms']) ? $form_state['values']['module_forms'] : array(),
     '#disabled' => $step < 3 ? FALSE : TRUE,
    );
 }

 if( $step == 3 && !isset($form_state['values']['content'])) {    
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
    if($measure) {
     $runs = 10;
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
    while($j<$k) {
     $j = $j+1;
     while($i<$runs) {
      $i = $i+1;
  
      $counter = 0;
      foreach ($actions as $tag => $entry) {
        $descr = $entry[PATTERNS_INFO];
        foreach ($entry as $action => $forms) { 
          if ($action != PATTERNS_INFO && $action != PATTERNS_FILES && $action != PATTERNS_CREATE && $action != PATTERNS_DELETE) {
            foreach ($forms as $my_form) {
              $key = "o".$counter;
              if ($chosen_forms[$key]) {
                // include needed files
                if (isset($entry[PATTERNS_FILES])) {
                 foreach($entry[PATTERNS_FILES] as $file) {
                   include_once  $file;
                 }
                }
                // check if we get arguments for several runs
                $loop = FALSE;
                if (function_exists($module_name . "_patterns_get_arguments"))
                    $arguments = call_user_func_array($module_name . "_patterns_get_arguments", array($action, $tag, $my_form,&$loop));
                else {
                    $arguments = array("");
                }
                if (!$loop) {
                  $arguments = array($arguments);
                }
                  
                foreach ($arguments as $arguments_element) {
                 // submit and save output
                 $module_form_state = array();
                 $module_form_state['values'] = array();
                 variable_set('macro_submissions', array());
                 
                 $module_form_state['build_info']['args'] = $arguments_element;
                 drupal_form_submit($my_form, $module_form_state);

                 $my_macro = macro_get_macro();
                 
                 $my_macro = substr($my_macro, 4, strlen($my_macro)-8);
                 $my_macro = "      " . str_replace("\n","\n      ", $my_macro);
                 $yaml_content[$action] .= "  - " . $action .":\n" .
                                           "      tag: ". $tag ."\n" .
                                           $my_macro . "\n\n";
                }
              }
              $counter++;
            }
          } 
        }
      }
  
     }

      if($measure) {
       $time = (microtime(TRUE) - $start);
       $min = $time < $min ? $time : $min;
       drupal_set_message("testing with " . $runs . " runs, k = ". $j . ", time is: ". $time);
      }
    }
    if($measure) 
      drupal_set_message("k-best: ". $min);
         variable_set('macro_enabled', FALSE);
      variable_set('patterns_import_module', FALSE);
     // creating info section of yaml pattern file
   $yaml = "# YAML ". $modules_with_components[$form_state['values']['module']] ." Pattern\n" .
         "\n" .
         "info:\n" .
         "  title: #please add a title\n" .
         "  description: " . 
              "automatically generated Pattern file";
   
   global $user;
   global $base_root;
   
   $yaml .= "\n" .
         "  author: ". $user->name ."\n" .
         "  category: ". $modules_with_components[$form_state['values']['module']] ."\n" .
         "  version: 1.0\n" .
         "  core: x.y\n" .
         "  author_email: ". $user->mail ."\n" .
         "  author_website: ". $base_root ."\n" .
         "\n" .
         "actions:\n" .
         "";
   // concatenating the actions
   $yaml .= $yaml_content[PATTERNS_CREATE];
   $yaml .= $yaml_content[PATTERNS_MODIFY];
   $yaml .= $yaml_content[PATTERNS_DELETE];
 }
 
 // show pattern import form
 if($step > 2) {
    $def_dir = patterns_config_get_files_dir();
    
    if(!isset($yaml))
      $yaml = $form_state['values']['content'];
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
    
    // TODO: transform in a FILE form
    $form['destination'] = array(
      '#type' => 'textfield',
      '#title' => t('Destination'),
      '#description' => t('Pattern will be saved in this directory. Must be writable. Default dir: @dir', array('@dir' => $def_dir)),
      '#default_value' => $def_dir,
      '#required' => TRUE,
    );
 }
 
 
 if($step <= 2) {
   $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Next Step'),
   );
   $form['#validate'][] = 'patterns_import_module_validate';
   $form['#submit'][] = 'patterns_import_module_submit';
 }
 else if($step > 2)  { // form elements needed to import yaml file    
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
 */
function patterns_form_alter(&$form, $form_state, $form_id) {
      // Add the record callback on submit and delete all others
      if ($form_id != 'macro_import_macro' && variable_get('macro_enabled', FALSE) && variable_get('patterns_import_module', FALSE)) {
        $form['#submit'] = array();
        $form['#submit'][] = 'macro_record_macro';
      }
}

/**
 * validation function for import_module_form
 * checks in which step we are and puts the step counter up and sets the rebuild flag
 */
function patterns_import_module_validate($form, &$form_state) {
 $form_state['rebuild'] = TRUE;
 $form_state['values']['step'] = $form_state['values']['step']+1;

 return TRUE;
}
/**
 * Extracts all possible patterns from the given module
 */
function patterns_import_module_submit($form, &$form_state) {

}

/**
 * Helper function to be called in one of the import validation handlers.
 */
function _patterns_imports_check_dirs($dir) {
  // Check Patterns Directory
  if (!_patterns_io_is_patterns_dir_ready($dir)) {
    $message = t("Destination folder does not exist or is not writable: %path<br />", array('%path' => $dir)); // TODO: move this out for performance
    $link = l(t('status report'), 'admin/reports/status');
    $message .= t('You might want to check the !link to ensure your files directory exists and is writable.', array('!link' => $link));
    form_set_error('pattern_file', $message);
    return FALSE;
  }
}

/**
 * Helper function to be called in one of the import validation handlers.
 */
function _patterns_imports_check_name($name = '') {
  // Name Validation
  $errors_on_name = _patterns_parser_analyze_pattern_name($name);
  if (!empty($errors_on_name)) {
    form_set_error('name', t('Errors on the name of the pattern: ' . implode(', ', $errors_on_name)));
    return FALSE;
  }

  if ($obj = patterns_db_load_pattern_from_name($name)) {
    form_set_error('name', t('A pattern file with the same name alrady exists: !name', array('!name' => $name)));
    return FALSE;
  }
}
