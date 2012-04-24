<?php
/**
 * @file
 * Functions, forms related to the Patterns editor.
 */

/**
 * Form constructor for the Quick Run form.
 *
 * @see patterns_quickrun_submit()
 * @ingroup forms
 */
function patterns_quickrun($form, &$form_state) {

  $content = '';

  $text = t('The quick run mode should be used only in development environment, for testing, or if you really know what you are doing.');
  patterns_forms_add_page_header($form, 'Important!', $text);

  $options_mode = array(
    'extend' => 'Extend',
    'runover' => 'Run-Over',
  );

  // Add the execution options
  $form = patterns_forms_get_execution_options($form);

  $form['validation_result'] = array(
    '#markup' => 'This pattern has not been validated yet.',
    '#prefix' => '<div id="validation_result">',
    '#suffix' => '</div>',
  );

  $form = patterns_forms_get_formats_selector($form);

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
    '#markup' => "<a href='#' id='validate_pattern'>" . t('Validate') . "</a>",
    '#suffix' => '</span> ',
  );

  $form['cancel'] = array(
    '#markup' => l(t('Back'), 'admin/patterns'),
  );

  return $form;
}

/**
 * Form submission handler for patterns_quickrun().
 *
 * @see patterns_quickrun()
 */

function patterns_quickrun_submit($form, $form_state) {

  // Parse the content of the textarea
  $pattern['pattern'] = patterns_parser_parse($form_state['values']['content'], $form_state['values']['format']);

  $pattern = patterns_get_pattern_obj($pattern);

  $params['run-subpatterns'] = 'always'; // TODO
  $params['quickrun'] = TRUE;

  return patterns_start_engine($pattern, $params, $form_state['values']['mode']);
}
