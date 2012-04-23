<?php
/**
 * @file
 * Export the components configuration to a Pattern file.
 *
 */

/**
 * TODO: finish Export documentation
 * TODO: code cleanup
 * TODO: Export through the Batch API
 */



function _patterns_export_build_input_name($module, $tag) {
  return 'ext[' . $module . '][options][' . $tag . ']';
}

function _patterns_export_goto($form, &$form_state, $step) {
  // MODULE SELECTED

  if ($step == 1) {
    return patterns_export_page1($form, $form_state);
  }

  if ($step == 2) {
    // TODO: We should check if the users selects more options, and if the functions
    // require extra parameters. For later, not we just continue.
    return $form;

    $tm_index = patterns_tagmodules_get_index();
    $tag = $form_state['values']['options'];
    $func_id = $form_state['values'][$tag];

    // Check if the function wants extra parameters
    // if (isset($tm_index[$tag]) && isset($tm_index[$tag][PATTERNS_EXPORT][$func_id])){
    //  if (isset($tm_index[$tag]))
    //}

    //if (isset($form_state['values']['module']) && !is_int($form_state['values']['module']))  {
    // return patterns_export_page2($form, $form_state);
    //}
  }


  // FORM SELECTED
  /*
  if ( $step == 3 && !isset($form_state['values']['content'])) {
    return patterns_export_page3($form, $form_state);
  }

  // show pattern import form
  if ($step = 4) {
    return patterns_export_page4($form, $form_state);
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
function _patterns_export_check_step(&$form_state, $validation = FALSE) {

  $step = FALSE;

  if (isset($form_state['values']['all']) && $form_state['values']['all'] == 0) {
    $step = 6; // Export ALL
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

function _patterns_export_if_invalid_step_goback($step = 0,
                                                  $back = '/admin/patterns/export',
                                                  $options = array()) {

  // TODO: Adjust
  if ($step < 1 || $step > 10) {
    drupal_set_message(t('Export could not proceed to the next step. Step %id not found.', array('%id' => $step)), 'error');
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

function patterns_export($form, &$form_state) {

  //$step = _patterns_export_check_step($form_state);
  //_patterns_export_if_invalid_step_goback($step);
//  $form['step'] = array(
//     '#type' => 'hidden',
//     '#title' => t('Pattern Identifier'),
//     '#description' => t('step'),
//     '#value' => $step,
//  );

  //debug($step);

  //drupal_add_js('misc/ajax.js');
  $name = isset($form_state['values']['name']) ?  $form_state['values']['name'] : 'name not set';

  patterns_io_load_components();

  return patterns_export_page1($form, $form_state);
  //return _patterns_export_goto($form, $form_state, $step);

}

function patterns_export_page1($form, &$form_state) {

  $tm_index = patterns_tagmodules_get_index();
  $tm_index = patterns_tagmodules_filter($tm_index, NULL, PATTERNS_EXPORT);

  if (count($tm_index) == 0) {
      drupal_set_message(t('The components currently installed do not allow automatic export of the configuration of the web site'), 'error');

      $form['disabled'] = array(
        '#markup' => l(t('Click here to back'), 'admin/patterns'),
      );

    return $form;
  }

  $text = 'In this area you can export the current configuration of your web site to a new pattern file that will be inserted in the database.';
  $title = 'Export (experimental!)';
  patterns_forms_add_page_header($form, $title, $text);

  $form['all'] = array(
    '#type' => 'fieldset',
    '#title' => 'Options',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE, // TODO set to TRUE after debugging
  );

  $form['all']['all'] = array(
    '#type' => 'fieldset',
    '#title' => 'I want to select the components individually (Experimental)',
    '#type' => 'checkbox',
  );

  $form['all']['ext'] = array(
    '#type' => 'fieldset',
    '#title' => 'Components with automatic export enabled',
    '#tree' => TRUE,
    '#collapsible' => TRUE,
    '#collapsed' => FALSE, // TODO set to TRUE after debugging
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

    $form['all']['ext'][$module]['options'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Export options'),
      '#options' => array_combine(array_keys($tags), array_keys($tags)),
    );

    foreach ($tags as $tag => $forms) {

      $input_name = _patterns_export_build_input_name($module, $tag);

      $form['all']['ext'][$module][$tag]['options'] = array(
        '#type' => 'checkboxes',
        '#title' => $tag,
        '#options' => array_combine(array_keys($forms), array_keys($forms)),
        '#states' => array(
           'visible' => array(   // action to take.
            ':input[name="' . $input_name . '"]' => array('checked' => TRUE),
            ),
           'checked' => array(   // action to take.
            ':input[name="' . $input_name . '"]' => array('checked' => TRUE),
            ),
        ),
      );
    }


    //_patterns_export_add_ajax_to_options($form['ext'][$module]);

  }

  patterns_forms_get_info_section($form, array(), array('collapsed' => TRUE));

  patterns_forms_get_formats_selector($form,
                                              patterns_forms_get_default_format(),
                                              'Export data in this format',
                                              FALSE
  );

  patterns_forms_get_name_selector($form);

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Export'),
  );
  $form['#validate'][] = 'patterns_export_validate';

  return $form;
}

function patterns_export_submit($form, &$form_state) {

  $tm_index = patterns_tagmodules_get_index();
  $format   = $form_state['values']['format'];
  $filename = $form_state['values']['pattern_name'];
  // TODO: Loop here and create a modules section


  // Sets the category for the info
  // category: " . $modules_with_components[$form_state['values']['module']]
  $pattern = patterns_api_add_info_section($form_state['values']['info']);

  if (isset($form_state['values']['all']) && $form_state['values']['all'] != 0) {
    // Individual modules have selected and the tagmodules index needs to be
    // refactored.
    $exports = array();
    foreach ($form_state['values']['ext'] as $module => $data) {

      foreach ($data['options'] as $tag => $enabled) {
        if (empty($enabled)) {
          continue;
        }
        $exports[$tag] = array();
        // Loop along all enabled export functions
        foreach ($data[$tag]['options'] as $export_key => $func) {
          if (!empty($func)) {
            $tm_index[$tag]['__to_export__'][] = $tm_index[$tag][PATTERNS_EXPORT][$func];
          }
        }

        // If no sub-tag was selected (should not be the case)
        // then we execute EXPORT_ALL (if available)
        if (empty($tm_index[$tag]['__to_export__'])) {
          if (isset($tm_index[$tag][PATTERNS_EXPORT][PATTERNS_EXPORT_ALL])) {
            $tm_index[$tag]['__to_export__'] = $tm_index[$tag][PATTERNS_EXPORT][PATTERNS_EXPORT_ALL];
          }
        }
      }
    }

    $tm_index = array_intersect_key($tm_index, $exports);
    //$tm_index = array_combine($tm_index, $exports);
  }

//  if (isset($form_state['values']['options']) && !empty($form_state['values']['options'])) {
//    $cmp = $form_state['values']['options'];
//    $tm_index = array($cmp => $tm_index[$cmp]);
//
//    if (isset($form_state['values'][$cmp]) && !empty($form_state['values'][$cmp])) {
//      $tag = $form_state['values'][$cmp];
//      $func = $tm_index[$cmp][PATTERNS_EXPORT][$tag];
//      $tm_index[$cmp][PATTERNS_EXPORT][PATTERNS_EXPORT_ALL] = $func;
//    }
//    //else {
//    //  $tm_index[$cmp] = array($tag => $tm_index[$cmp][PATTERNS_EXPORT_ALL]);
//    //}
//
//  }

  list($sections, $modules) = patterns_export_php($tm_index);

  if (empty($sections)) {
    drupal_set_message(t('Export did not return any data.'), 'error');
    return FALSE;
  }

  // Modules that needs to be enabled
  // to run the pattern
  patterns_api_add_modules_section($modules, $pattern);

  foreach ($sections as $s => $values) {
    $pattern[$s] = $values;
  }

  // Display the exported pattern
  //drupal_set_message(print_r($pattern, true));

  $result = patterns_io_save_pattern($pattern, $filename, $format);

  if ($result) {
    drupal_set_message(t('Pattern succesfully imported.'));
  }


  //patterns_export_batch($tm_index);

  $form_state['redirect'] = 'admin/patterns/export';

}
function patterns_export_php($tagmodules) {

  $result = array();
  $modules = array();
  foreach ($tagmodules as $tag => $values) {
    if (isset($values['__to_export__'])) {
        $values[PATTERNS_EXPORT][PATTERNS_EXPORT_ALL] = $values['__to_export__'];
    }
    elseif (!isset($values[PATTERNS_EXPORT]) || !isset($values[PATTERNS_EXPORT][PATTERNS_EXPORT_ALL])) {
        continue;
    }

    if (isset($values[PATTERNS_FILES])) {
      $files = (!is_array($values[PATTERNS_FILES])) ? array($values[PATTERNS_FILES]) : $values[PATTERNS_FILES];
      foreach ($files as $file) {
        require_once $file;
      }
    }

    array_push($modules, $values['module']);
    $func = $values[PATTERNS_EXPORT][PATTERNS_EXPORT_ALL];
    if (!is_array($func)) {
      $sections[$tag] = call_user_func($func);
    }
    else {
      foreach ($func as $f) {
          $result[$tag] = isset($result[$tag]) ? $result[$tag] : array();
        $sections[$tag] = array_merge($result[$tag], call_user_func($f));
      }
    }

  }

  $result[] = $sections;
  $result[] = $modules;

  return $result;
}


/**
 * validation function for import_module_form
 * checks in which step we are and puts the step counter up and sets the rebuild flag
 */
function patterns_export_validate($form, &$form_state) {

  //$step = _patterns_export_check_step($form_state, TRUE);
  //if ($step == 1) {
  //  return _patterns_export_validate_step1($form, $form_state);
  //}

//  shortcut_set Array [1]
//  [0...0]
//    options Array [1]
//      [0...0]
//        shortcut_set  Array [1]
//          [0...0]
//            All 0
//
//
//
//  taxonomy Array [3]
//  [0...2]
//    options Array [2]
//      [0...1]
//        vocabulary  vocabulary
//        term  term
//    vocabulary  Array [1]
//      [0...0]
//        options Array [1]
//          [0...0]
//            vocabulary  Array [2]
//              [0...1]
//                All All
//                Cheat (I am doing terms)  0
//    term  Array [1]




  //$form_state['values']['step'] = $form_state['values']['step']+1;

  return TRUE;
}

function _patterns_export_validate_step1($form, &$form_state) {

  if (!isset($form_state['values']['format'])) {
    form_set_error('format', t('No valid format selected.'));
    return FALSE;
  }

  if (isset($form_state['values']['all']) && $form_state['values']['all'] == 0) {
    return TRUE; // We export all, no need for further checkings
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
