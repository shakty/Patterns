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
  
  $form = patterns_forms_get_formats_selector($form);
  
//  $form['format'] = array(
//    '#type' => 'select',
//    '#title' => t('Pattern syntax'),
//    '#options' => array_combine(patterns_config_get_formats(), patterns_config_get_formats()),
//    '#default_value' => PATTERNS_FORMAT_UNKNOWN,
//    '#description' => t('Pattern will be validated and run against this format.'),
//  );
  
  $form['validation_result'] = array(
    '#markup' => 'This pattern has not been validated yet.',
    '#prefix' => '<div id="validation_result"><strong>',
    '#suffix' => '</strong></div>',
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
  
  $form['validate'] = array(
    '#prefix' => '<span style=\'margin-right:10px\'>',
    '#markup' => "<a href='#' id='validate_pattern'>" . t('Validate') . "</a>",
    '#suffix' => '</span> ',
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
  
  if (!ini_get('allow_url_fopen')) {
    $form['import_url'] = array(
    // The string 'Feature disabled' is checked inside the test directory
    // do not change or change both
    '#prefix' => '<strong>' . t('Feature disabled:') . '</strong>',
    '#markup' => t('directive "allow_url_fopen" must be enabled in your PHP configuration in order to use this feature.'),
    );
    return $form;
   }
  
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

  form_set_error('pattern_uri', t('Failed to retrieve the pattern specified: %uri. Check your file and try again. Notice: only yaml and xml files are accepted', array('%uri' => $uri)));
  return FALSE;
 }

 $format = pathinfo($form_state['pattern_uri']->uri, PATHINFO_EXTENSION);

 // TODO: PHP format
 if ($format == 'yaml') {
  // YAML
  $patternscan = _patterns_parser_scan_pattern($form_state['values']['content']);
  if (!_patterns_parser_validate_patternscan($patternscan)) {
   $errors_on_scan = _patterns_parser_analyze_patternscan($patternscan);
   form_set_error('name', t('Error parsing the YAML file: ' . implode(', ', $errors_on_scan))); // TODO: proper t()
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

  $format = $form_state['values']['format'];
  
  $destination = $form_state['values']['destination'];
  
  // If not automatically returns
  _patterns_imports_check_dirs($destination);
  _patterns_imports_check_name($form_state['values']['name']);
  
   // It is not an import_source
  if (!isset($form_state['values']['content'])) {
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
     form_set_error('pattern_uri', t('Failed to retrieve the pattern specified: %uri. Check your file and try again. Notice: only yaml and xml files are accepted', array('%uri' => $uri)));
     return FALSE;
    }
  }

  // FORMAT Validation
  if ($format == PATTERNS_FORMAT_XML) {
    $pattern = patterns_load_string_xml($form_state['values']['content']); 
  }
  else if ($format == PATTERNS_FORMAT_YAML)  {
    $pattern = patterns_parse_yaml($form_state['values']['content']);
  }
  else {
    // PHP or UNKNOWN
    $pattern = $form_state['values']['content'];
  }
  
 
  $patternscan = _patterns_parser_scan_pattern($pattern);
  if (!_patterns_parser_validate_patternscan($patternscan)) {
    $errors_on_scan = _patterns_parser_analyze_patternscan($patternscan);
    form_set_error('name', t('Error parsing the YAML file: ' . implode(', ', $errors_on_scan))); // TODO: proper t()
  }
 

  // We save here the parsed pattern
  $form_state['values']['pattern'] = $pattern;
  
  
  return TRUE;

}

function patterns_import_submit($form, &$form_state) {

  $success = FALSE;
  $name = $form_state['values']['name'];
  
  $format = (isset($form_state['values']['format'])) ? $form_state['values']['format'] : PATTERNS_FORMAT_UNKNOWN;
  
  $content = (isset($form_state['values']['pattern'])) ? $form_state['values']['pattern']
                                                      : $form_state['values']['content'];
  
  if (!empty($content)) {
    $success = patterns_io_save_pattern($content, 
                                        $name, 
                                        $format, 
                                        $form_state['values']['destination'], 
                                        $form_state['values']['content']
    );
  }
  
  if ($success) {
    $link = l('here', 'admin/patterns/edit/' . $name);
    $msg = t('Pattern %name successfully imported. ', array('%name' => $name));
    $msg.= t('Click ') . $link . t(' to view it.'); 
    drupal_set_message($msg);
  }
  else {
    drupal_set_message(t('File %name could not be saved on the server. Import failed.', array('%name' => $name)), 'error');
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
          } else {
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
   $def_dir = patterns_config_get_files_dir();

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
 * Helper function to be called in one of the import validation handlers.
 */
function _patterns_imports_check_dirs($dir = NULL) {
 // Check Patterns Directory
 if (!_patterns_io_is_patterns_dir_ready($dir, FILE_CREATE_DIRECTORY)) {
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
  form_set_error('name', t('Errors in the name of the pattern: ' . implode(', ', $errors_on_name))); // TODO: use proper t()
  return FALSE;
 }

 if ($obj = patterns_db_load_pattern_from_name($name)) {
  form_set_error('name', t('A pattern file with the same name already exists: !name', array('!name' => $name)));
  return FALSE;
 }
}

/**
 * Creates a miniminal form_state object to import a pattern into the database.
 *
 * Returns FALSE if $name or $pattern are missing.
 *
 * @param mixed $name the name of the pattern
 * @param mixed $pattern the pattern code as string or array. If array gets
 *  converted to string
 * @param mixed $dest the folder in the file system where the pattern will be
 *  saved. If empty it uses the default patterns files directory.
 */
function patterns_import_build_import_form_state($name = NULL, $pattern = NULL, $dest = NULL) {
  if (is_null($name) || is_null($pattern)) return FALSE;
  if (is_null($dest)) {
    $dest = patterns_config_get_files_dir();
  }

  $form = array();
  $form['values'] = array();
  $form['values']['name']         = $name;
  $form['values']['content']      = $pattern;
  $form['values']['destination']  = $dest;
  return $form;
}

