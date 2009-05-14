<?php

function patterns_patterns($op, $id = null, &$data = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('pattern');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'patterns_enable_pattern' => t('Patterns: Enable pattern'),
      );
    break;

    // Return a summary of an action
    case 'summary':
      return t('Enabling pattern %pattern', array('%pattern' => $data));
    break;

    // Prepare data for processing
    case 'prepare':
    break;

    // Pre validate actions
    case 'pre-validate':
    break;

    // Return the form_id('s) for each action
    case 'form_id':
      $status = db_result(db_query('SELECT status FROM {patterns} WHERE name = "%s"', $data['value']));
      $form_id = $status ? '' : 'patterns_enable_pattern';

      return $form_id;
    break;

    // Prepare for valid processing of this type of component
    case 'build':
      $data['op'] = t('Confirm');
      $data['confirm'] = 1;

      return $data;
    break;

    // Validate the values for an action before running the pattern
    case 'validate':
      if (!patterns_get_pattern($data['value'])) {
        return t("Pattern %pattern doesn't exist or it's not valid.", array('%pattern' => $data['value']));
      }
    break;

    // Build a patterns actions and parameters
    case 'params':
      $pattern = patterns_get_pattern($data['value']);

      return $pattern->pid;
    break;
  }
}
