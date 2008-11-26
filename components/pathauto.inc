<?php

function pathauto_patterns($op, $id = null, &$data = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('pathauto');
    break;
    
    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'pathauto_admin_settings' => t('Setup pathauto'),
      );
    break;
    
    // Return a summary of an action
    case 'summary':
      return t('Setup pathauto urls');
    break;
    
    // Prepare data for processing
    case 'prepare':
      foreach($data as $key => $value) {
        if (is_array($value)) {
          $mappings = array('default' => 'pattern');
          $skip = array('bulkupdate', 'applytofeeds');
          foreach($value as $i => $v) {
            if (in_array($i, $skip)) {
              $new = $key .'_'. $i;
            }
            else if (array_key_exists($i, $mappings)) {
              $new = $key .'_'. $mappings[$i];
            }
            else {
              $new = $key .'_'. $i .'_pattern';
            }
            
            $data[$new] = $v;
            unset($data[$key][$i]);
          }
          unset($data[$key]);
        }
      }
      
      $mappings = array('update' => 'update_action');
      foreach($data as $key => $value) {
        if (array_key_exists($key, $mappings)) {
          $new = $mappings[$key];
        }
        else {
          $new = 'pathauto_'. $key;
        }
        
        if (strpos($key, 'pathauto_') === FALSE) {
          $data[$new] = $value;
          unset($data[$key]);
        }
      }
    break;
    
    // Pre validate actions
    case 'pre-validate':
    break;
    
    // Return the form_id('s) for each action
    case 'form_id':
      module_load_include('inc', 'pathauto', 'pathauto.admin');
      return 'pathauto_admin_settings';
    break;
    
    // Prepare for valid processing of this type of component
    case 'build':
      if ($data['reset']) {
        foreach($data as $key => $value) {
          $data[$key] = '';
        }
        
        unset($data['reset']);
      }
      
      return $data;
    break;
    
    // Validate the values for an action before running the pattern
    case 'validate':
    break;
    
    // Build a patterns actions and parameters
    case 'params':
    break;
    
    // Reverse actions when disabling a pattern
//     case 'reverse':
//       $data['reset'] = true;
//     break;
    
    // Cleanup any global settings after the action runs
    case 'cleanup':
      unset($_POST['op']);
    break;
  }
}
