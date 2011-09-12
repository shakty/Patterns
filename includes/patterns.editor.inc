<?php

/**
 * Builds up a pattern editing environment. Loads additional javascript libraries,
 * and supplies ajax validation on the fly.
 * 
 * @param mixed $pattern
 * 
 */
function patterns_editor($form_id, $pattern = array()) {

  $pattern = patterns_get_pattern_obj($pattern);
 
  $form = drupal_get_form($form_id, $pattern);
  $editor  = drupal_render($form);
  
  if (!patterns_load_co2()) {
    $co2 = l('Codemirror', 'http://codemirror.net/');
    $editor .= t('Install !co2 in sites/all/libraries/codemirror2 for advanced editing functionalities.', array('!co2'=>$co2));
  }
  
  // Load the JS of for the ajax validation
  drupal_add_js(drupal_get_path('module', 'patterns') . '/js/ajax_validation.js');
  
  return $editor;
}

function patterns_quickrun () {
  // TODO: this form could be reused, see function patterns_enable_pattern().
  // Default
  $content = "# Pattern\n";
  $extension = 'yaml';
  
  $form['important'] = array (
    '#markup' => '<strong>Important!</strong> The quick run mode should be used only in development environment, for testing, or if you really know what you are doing.',
    '#prefix' => '<div id="important">',
    '#suffix' => '<hr><br/></div>',
  );
  
  $form['validation_result'] = array(
    '#markup' => 'This pattern has not been validated yet.',
    '#prefix' => '<div id="validation_result">',
    '#suffix' => '</div>',
  );
  $options_mode = array(
    'extend' => 'Extend',
    'runover' => 'Run-Over',
  );
  
  $form['execution'] = array(
    '#type' => 'radios',
    '#title' => t('Execution mode:'),
    '#description' => t('Extend mode can only add new feature to your web-site, Run-over can also delete. At the moment \'Extend\' is the only mode permitted.'),
    '#options' => $options_mode,
    '#default_value' => 'extend',
    '#disabled' => 'true',
  );

  $options_run_mode = array(
    'batch' => 'Batch',
    'php' => 'PHP',
  );

  $form['mode'] = array(
    '#type' => 'radios',
    '#title' => t('Running mode:'),
    '#description' => t('Batch mode uses the Batch API provided by Drupal thus providing intermediate feedback about the progress. PHP will only return when every action has been processed (or there has been an error), which is easier to debug.'),
    '#options' => $options_run_mode,
    '#default_value' => 'batch',
  );
  
  $form['format'] = array(
    '#type' => 'select',
    '#title' => t('Pattern syntax'),
    '#options' => array_combine(patterns_file_types(), patterns_file_types()),
    '#default_value' => $extension,
  );
  
  $form['content'] = array(
    '#type' => 'textarea',
    '#title' => t('Pattern\'s code'),
    '#rows' => 25,
    '#default_value' => $content,
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Run'),
  );
    
  $form['validate'] = array(
    '#prefix' => '<span style=\'margin-right:10px\'>',  
    '#markup' => "<a href='#' id='validate_pattern'>" .t('Validate'). "</a>",
    '#suffix' => '</span> ',
  );
  
  $form['cancel'] = array(
    '#markup' => l(t('Cancel'), 'admin/patterns'),
  );  
  
  return $form;
}

function patterns_quickrun_submit($form, $form_state) {

  // Parse the content of the textarea
  $pattern['pattern'] = patterns_parse_yaml($form_state['values']['content']); // TODO: other formats as well

  $pattern = patterns_get_pattern_obj($pattern);

  // TODO: is this checking needed?
  if(!in_array($form_state['values']['mode'], array('batch', 'php'))) {
  	$mode = 'batch';
  }
  else {
  	$mode = $form_state['values']['mode'];
  }

  $params['run-subpatterns'] = 'always'; // TODO
  $params['quickrun'] = TRUE;

  return patterns_start_engine($pattern, $params, $mode);
}
/**
 * Display the page for create a new pattern.
 * 
 */
function patterns_create_page($pid = NULL) {
  
  $info = array();
  $pattern = array();
  
  if (is_numeric($pid)) {
    $pattern = patterns_get_pattern($pid); 
  
    if ($pattern->content) {
      $info['Title'][] = t('Title:');
      $info['Title'][] = $pattern->title;
      $info['Path'][] = t('Path:');
      $info['Path'][] = $pattern->file;  
    }
  }
  $out = '<h6>Info</h6>';
  $out.= theme('table', array('rows' => $info, 'attributes' => array('class' => 'patterns-list')));  
  $out.= patterns_editor('patterns_edit', $pattern);

  return $out;
}

/*
 * Display the page for editing patterns.
 * 
 */
function patterns_edit_page ($pid = NULL) {
  if (!is_numeric($pid)) {
    drupal_set_message(t('You must specify a pattern to edit.'));
    return FALSE;
  }
  
  $pattern = patterns_get_pattern($pid);
  
  if (!$pattern->file) {
    drupal_set_message(t('This pattern does not seem to be associated with a valid source file.'), 'error');
    return FALSE;
  }
  
  $info = array();
  $info['Path'][] = t('Path:');
  $info['Path'][] = $pattern->file;
  $info['Enabled'][] = t('Enabled:');
  $info['Enabled'][] = ($pattern->enabled) ? t('Yes') : t('No');
  
//  $info['Edited'][] = t('Edited:');
//  
//  $info['Edited'][] = (variable_get(PATTERNS_FAILED_EDIT) == '' ) ?  
//                                                                   t('Latest version of the pattern displayed.') :
//                                                                   t('This is a NOT saved, edited version of the original pattern!!') ;
                                                                      
  
  

  $out = '<h6>Info</h6>';
  $out.= theme('table', array('rows' => $info, 'attributes' => array('class' => 'patterns-list')));  
  $out.= patterns_editor('patterns_edit', $pattern);
  
  return $out;
}


/**
 * Create the form to edit patterns
 */
function patterns_edit($form, &$form_state, $pattern, $full = TRUE) {
// TODO: this form could be reused, see function patterns_enable_pattern().
  // Default
  $content = '# Pattern';
  $extension = 'yaml';
  
  if (!empty($pattern->file)) {
    $content = file_get_contents($pattern->file);
  }

  if (!empty($pattern->file)) { 
    $extension = pathinfo($pattern->file, PATHINFO_EXTENSION);
  }

  $form['name'] = array(
    '#type' => 'value',
    '#value' => $pattern->name,
  );
  
  $form['pid'] = array(
    '#type' => 'value',
    '#value' => $pattern->pid
  );

  $form['validation_result'] = array(
    '#markup' => 'This pattern has not been validated yet.',
    '#prefix' => '<div id="validation_result">',
    '#suffix' => '</div>',
  );

  $form['format'] = array(
    '#type' => 'select',
    '#title' => t('Pattern syntax'),
    '#options' => array_combine(patterns_file_types(), patterns_file_types()),
    '#default_value' => $extension,
  );
  
  $form['content'] = array(
    '#type' => 'textarea',
    '#title' => t('Pattern\'s code'),
    '#rows' => 25,
    '#default_value' => $content,
  );
  
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
    
  $form['validate'] = array(
    '#prefix' => '<span style=\'margin-right:10px\'>',  
    '#markup' => "<a href='#' id='validate_pattern'>" .t('Validate'). "</a>",
    '#suffix' => '</span> ',
  );

  $form['run'] = array(
    '#prefix' => '<span style=\'margin-right:10px\'>',  
    '#markup' => l(t('Run current pattern'), 'admin/patterns/enable/' . $pattern->pid),
    '#suffix' => '</span> ',
  );
  
  $form['cancel'] = array(
    '#markup' => l(t('Cancel'), 'admin/patterns'),
  );  

   //$content = variable_get(PATTERNS_FAILED_EDIT,file_get_contents($pattern->file));

//   if ($pattern->enabled) {
//    $form['enable'] = array(
//      '#type' => 'markup',
//      '#value' => l(t('Undo update changes to the state when you enabled the pattern.'), 'admin/patterns/revert/'. $pid, array(), drupal_get_destination())
//    );
//  }
    
  
  //    '#ajax' => array(
//      'callback' => 'patterns_validate_service',
//      'wrapper' => 'replace_textfield_div',
//      'effect' => 'fade',
//     ),
//  );
  
//  $form['submit'] = array(
//    '#type' => 'button',
//    '#validate' => array(),
//    '#submit' => array('patterns_edit_submit_drop_mod'),
//    '#value' => t('Drop Modifications'),
//  );
  
// CHECK THIS  
//  $form['submit'] = array(
//    '#type' => 'submit',
//    '#submit' => array('patterns_edit_submit_save');
//    '#value' => t('Save Modifications'),
//  );
  
//  $form['run'] = array(
//    '#prefix' => '<a href='.url('admin/patterns/enable/'.$pattern->pid).'>',
//    '#type' => 'button',
//    '#value' =>  t('Run saved pattern'),
//    '#suffix' => '</a>',
//  );
  
  return $form;
}

//function patterns_edit_validate_save($form, &$form_state) {
//  return true;
//}
//
//function patterns_edit_submit_save($form, &$form_state) {
//  drupal_goto('admin/patterns/enable/'.$pattern->pid);
//}


/**
 * Validate pattern modifications (still to do...)
 */
function patterns_edit_validate($form, &$form_state) {
  
  // real_path(public) returns the path to the files dir
  // TODO: patterns_save_file is never set.
  $path = drupal_realpath('public://') . "/" . variable_get('patterns_save_file', 'patterns');

  if (!file_prepare_directory($path, FILE_CREATE_DIRECTORY)) { // TODO: is FILE_CREATE_DIRECTORY good here?
    form_set_error('form_token', t('Unable to create @path to save the new pattern to.', array('@path' => $path)));
  }
}

/**
 * Submit edits to the pattern
 */
function patterns_edit_submit($form, &$form_state) {
  
  $name = $form_state['values']['name']; 
  $content = $form_state['values']['content'];
  $format = $form_state['values']['format'];
  
  patterns_io_save_pattern ($content, $name, $format);
 
  //variable_set(PATTERNS_FAILED_EDIT,$form_state['values']['content']);
  
}

///**
// * Submit edits to the pattern
// */
//function patterns_edit_submit($form, &$form_state) {
//  // If this is an enabled pattern, make sure the enabled pattern is saved in its current state
//  
//  $tmp = $form_state['values']['name'];
//  
//  // Ste: Removed STATUS = 1: Why should it matter?
//  if ($results = db_query("SELECT file,status FROM {patterns} WHERE name = :name", array('name' => $form_state['values']['name']))->fetchAssoc()) {
//  
//    $file    = $results['file'];
//    $enabled = $results['status'];
//    
//    
//    $path_parts = pathinfo($file);
//
//      
////    echo $path_parts['basename'], "\n";
////    echo $path_parts['extension'], "\n";
////    echo $path_parts['filename'], "\n";
//    
//    // was
//    $dir = drupal_realpath('public://') . "/" . variable_get('patterns_save_file', 'patterns');
//
//    //$dir = drupal_realpath('public://') . $path_parts['dirname'];
//    
//    if ($enabled) {
//      $dir.= '/enabled';
//    }
//    
//    //@TODO: if enabled take care of that
//    
//    
//    if (file_prepare_directory($dir, FILE_CREATE_DIRECTORY)) {
//      $path =  $dir . '/' . $form_state['values']['name'] . '.' . $form_state['values']['format'];
//
//
//        // Save the new pattern into the pattern files dir.
//      // $path = drupal_realpath('public://') . '/' . variable_get('patterns_save_file', 'patterns') . '/' . $form_state['values']['name'] . '.' . $form_state['values']['format'];
//    
//      if (file_unmanaged_save_data($form_state['values']['content'], $path, FILE_EXISTS_REPLACE)) {
//    
//        $old = db_query("SELECT file FROM {patterns} WHERE name = :name", array('name' => $form_state['values']['name']))->fetchField();
//        
//        // Load and save pattern
//        $load_func = 'patterns_load_' . $form_state['values']['format']; 
//        
//        if ($pattern = $load_func($path)) {
//          if ($old) {
//            $query_params = array('file' => $path, 'updated' => time(), 'pid' => $form_state['values']['pid']);
//            db_query("UPDATE {patterns} SET file = :file, updated = :updated WHERE pid = :pid", $query_params);
//          }
//          patterns_save_pattern($pattern, $path, $form_state['values']['name']);
//          drupal_set_message(t('%name was saved.', array('%name' => $form_state['values']['name'])));
//          $form_state['redirect'] = 'admin/patterns';
//          // Not for now
//          //_patterns_clear_failed_edit();
//        }
//        else {
//          drupal_set_message(t("Pattern '%name' couldn't be saved. Make sure edited code is well-formed.", array('%name' => $form_state['values']['name'])), 'error');
//        }
//      }
//      else{
//        drupal_set_message(t('Error: file could not be saved.'), 'error');
//      }
//    }
//    else {
//      drupal_set_message(t('Error: the pattern is not writable. Please check the file system permissions.'), 'error');
//    }
//  }
//  else {
//    drupal_set_message(t('Error: file not found ?!.'), 'error');
//  }
//
//  //variable_set(PATTERNS_FAILED_EDIT,$form_state['values']['content']);
//  
//}


/**
 * 
 * Adds the javascript files to load the Co2 Editing TextArea
 * 
 */
function patterns_load_co2 () {
  
// Load the CodeMirror Editor if installed in the libraries folder
  $co2_path = libraries_get_path('codemirror2');
  if (!empty($co2_path)) {
    $co2_js = $co2_path . '/lib/codemirror.js';
    $co2_css = $co2_path . '/lib/codemirror.css';
    $co2_mode_yaml = $co2_path . '/mode/yaml/yaml.js';
    $co2_css_theme = $co2_path . '/theme/default.css';
    
    
    drupal_add_css($co2_css);
    drupal_add_css($co2_css_theme);
    drupal_add_js($co2_js);
    drupal_add_js($co2_mode_yaml);
    
    // Adding Pattern Customization for Co2
    drupal_add_css(drupal_get_path('module', 'patterns').'/css/editor.css');
    drupal_add_js(drupal_get_path('module', 'patterns').'/js/editor.js');
    
    return TRUE;
  }
  
  return FALSE;
  
}