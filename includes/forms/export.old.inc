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
    //  if (isset($tm_index[$tag]))
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
 *   The current value of the step, or FALSE if it fails to determine it
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


  $tm_index = patterns_api_build_tagmodules_index(NULL, NULL, PATTERNS_EXPORT);

  if (count($tm_index) == 0) {
      drupal_set_message(t('The components currently installed do not allow automatic extraction of the configuration of the web site'), 'error');

      $form['disabled'] = array(
        '#markup' => l(t('Click here to back'), 'admin/patterns'),
      );

    return $form;
  }

  $form['all'] = array(
    '#type' => 'fieldset',
    '#title' => 'Options',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE, // TODO set to TRUE after debugging
  );

  $form['all']['all'] = array(
    '#type' => 'fieldset',
    '#title' => 'I want to extract configuration from single components (Experimental)',
    '#type' => 'checkbox',
  );

  $form['all']['ext'] = array(
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

    $form['all']['ext'][$module] = array(
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
  $format   = $form_state['values']['format'];
  $filename = $form_state['values']['pattern_name'];
  // TODO: Loop here and create a modules section


  // Sets the category for the info
  // category: " . $modules_with_components[$form_state['values']['module']]
  $pattern = patterns_api_add_info_section($form_state['values']['info']);

  if (isset($form_state['values']['options'])) {
    $cmp = $form_state['values']['options'];
    $tm_index = $tm_index[$cmp];
    if (isset($form_state['values'][$cmp])) {
      $tag = $form_state['values'][$cmp];
      $tm_index[$tag];
    }
    else {
      $tm_index = $tm_index[PATTERNS_EXPORT_ALL];
    }
  }

  $sections = patterns_extract_php($tm_index);
  foreach ($sections as $s => $values) {
    $pattern[$s] = $values;
  }
  //$pattern = array_merge($info, $sections);

  //if (patterns_validate_pattern($pattern, ))

  //$parsed = patterns_parser_parse($pattern, $format);
  drupal_set_message(print_r($pattern, TRUE));

  if ($parsed) {
    drupal_set_message(t('Pattern succesfully extracted'));
  }

  $result = patterns_io_save_pattern($pattern, $filename, $format);

  if ($result) {
    drupal_set_message(t('Pattern succesfully imported'));
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
        $batch['file'] = drupal_get_path('module', 'patterns_components') . 'components/taxonomy.inc';
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

  $exports = patterns_api_build_moduletags_index($form_state['values']['module'], NULL, PATTERNS_EXPORT);
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

  if (!isset($form_state['values']['format'])) {
    form_set_error('format', t('No valid format selected.'));
    return FALSE;
  }

  if (isset($form_state['values']['all']) && $form_state['values']['all'] == 0) {
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

  if (!isset($form_state['values']['options'])) {
    form_set_error($module, t('No valid component selected.'));
    return FALSE;
  }

  $component = $form_state['values']['options'];
  if (!isset($form_state['values'][$component])) {
    form_set_error($module, t('No valid tag selected for component %component selected.', array('%component' => $component)));
    return FALSE;
  }

  return TRUE;
}


// NOT USED
////////////////////////////////////////////////////////





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

  $form['step_process']['step'] = array(
    '#markup' => $txt,
  );

  return FALSE;
}

function patterns_import_module_ajax_module_forms_callback($form, $form_state) {
  return $form['module_forms_container'];
}



function _patterns_extract_add_ajax_to_options(&$form) {
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


/**
 * Forms to create a yaml file based on a module
 * longer description:
 * Displays forms to choose a module (which implements the pattern hooks) and
 * the forms which we get from module_patterns,
 * then submits the forms and captures the form submissions with the Macro module.
 * From this, yaml file gets generated.
 *
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
