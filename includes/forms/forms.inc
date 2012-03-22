<?php

/**
 * Builds up a fieldset containing the options for executing a 
 * pattern. 
 * 
 */
function patterns_forms_get_execution_options($form = array()) {
	
	$form['options'] = array(
	    '#type' => 'fieldset',
	    '#title' => t('Options'),
	    '#collapsible' => TRUE,
	    '#collapsed' => TRUE,
	);
	
  $options_mode = array(
    'extend' => t('Extend'),
    'runover' => t('Run-Over'),
  );

  $form['options']['execution'] = array(
    '#type' => 'radios',
    '#title' => t('Execution mode:'),
    '#description' => t('Extend mode can only add new feature to your web-site, Run-over can also delete. At the moment \'Extend\' is the only mode permitted.'),
    '#options' => $options_mode,
    '#default_value' => 'extend',
    '#disabled' => 'true',
  );

  $options_run_mode = array(
    'batch' => t('Batch'),
    'php' => t('PHP'),
  );

  $form['options']['mode'] = array(
    '#type' => 'radios',
    '#title' => t('Running mode:'),
    '#description' => t('Batch mode uses the Batch API provided by Drupal thus providing intermediate feedback about the progress. PHP will only return when every action has been processed (or there has been an error), which is easier to debug.'),
    '#options' => $options_run_mode,
    '#default_value' => variable_get('patterns_default_running_mode', 'batch'),
    '#element_validate' => array('patterns_mode_validate'),
  );

  $options = array(PATTERNS_INCLUDE_NEVER => t('Never run'),
                   PATTERNS_INCLUDE_FIRSTRUN => t('Only during the first run'),
                   PATTERNS_INCLUDE_UPDATE => t('Only if pattern was updated'),
                   PATTERNS_INCLUDE_FIRSTRUN_OR_UPDATE => t('If it is the first run or if the pattern was updated'),
                   PATTERNS_INCLUDE_ATTACHED => t('Only if the pattern code is attached (no lookup to database)'),
                   PATTERNS_INCLUDE_ALWAYS => t('Always'),
  );

  $form['options']['run-subpatterns'] = array(
    '#type' => 'radios',
    '#title' => t('Run sub-patterns:'),
    '#description' => t("Decide when to run sub-patterns that are called by the currently run pattern. If unsure, stick to recommended setting. Note that your choice won't have any effect if your pattern doesn't contain sub-patterns or if this setting has been defined within the pattern file itself."),
    '#options' => $options,
    '#default_value' => patterns_get_include_mode(),
  );
	  
  return $form;
}

function patterns_forms_get_formats_selector($form = array(), $default = PATTERNS_FORMAT_UNKNOWN) {
	  
	$form['format'] = array(
    '#type' => 'select',
    '#title' => t('Pattern syntax'),
    '#options' => array_combine(patterns_config_get_formats(), patterns_config_get_formats()),
    '#default_value' => $default,
  );
  
  return $form;
}

