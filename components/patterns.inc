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
        'patterns_enable_pattern' => t('Enable pattern'),
//         'patterns_disable_pattern' => t('Disable pattern'),
      );
    break;
    
    // Return a summary of an action
    case 'summary':
//       if ($id == 'patterns_enable_pattern') {
        return t('Enabling pattern %pattern', array('%pattern' => $data));
//       }
//       else {
//         return t('Disabling pattern %pattern', array('%pattern' => $data['value']));
//       } 
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
//       if ($data['delete']) {
//         if ($status) {
//           return 'patterns_disable_pattern';
//         }
//         else {
//           return;
//         }
//       }
//       else {
        if (!$status) {
          return 'patterns_enable_pattern';
        }
        else {
          return;
        }
//       }
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
        return t('Invalid pattern');
      }
    break;
    
    // Build a patterns actions and parameters
    case 'params':
      $pattern = patterns_get_pattern($data['value']);
      
      return $pattern->pid;
    break;
    
    // Reverse actions when disabling a pattern
//     case 'reverse':
//       if ($data['delete']) {
//         unset($data['delete']);
//       }
//       else {
//         $data['delete'] = true;
//       }
//       return $data;
//     break;
  }
}
