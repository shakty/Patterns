<?php

function listmanager_patterns($op, $id = null, &$data = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('listmanager');
    break;
    
    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'listmanager_settings' => t('Setup a List Manager'),
        'listmanager_mappings_form' => t('Setup List Manager Profile Mappings')
      );
    break;
    
    // Prepare data for processing
    case 'prepare':
      $mappings = array('path' => 'listmanager_path', 'username' => 'listmanager_username', 'password' => 'listmanager_password', 'api' => 'listmanager_lmapi', 'lists' => 'listmanager_lists');
      foreach($data as $key => $value) {
        if (array_key_exists($key, $mappings)) {
          $data[$mappings[$key]] = $value;
          unset($data[$key]);
        }
      }
    break;
    
    // Pre validate actions
    case 'pre-validate':
    break;
    
    // Return the form_id('s) for each action
    case 'form_id':
      $mappings = array_intersect(array_flip(array('listmanager_path', 'listmanager_username', 'listmanager_password', 'listmanager_lmapi', 'listmanager_lists')), $data);
      
      if (!empty($mappings)) {
        if ($data['mappings']) {
          return array('listmanager_settings', 'listmanager_mappings_form');
        }
        else {
          return 'listmanager_settings';
        } 
      }
      else if ($data['mappings']) {
        return 'listmanager_mappings_form';
      }
    break;
    
    // Prepare for valid processing of this type of component
    case 'build':
      if ($id == 'listmanager_settings') {
        if ($data['reset']) {
          $_POST['op'] = t('Reset to defaults');
        }
      }
      else if ($id == 'listmanager_mappings_form') {
        $mappings = variable_get('listmanager_mappings', null);
        $data['mappings'] = array_merge($data['mappings'], (array)$mappings);
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
    case 'reverse':
      if (!$data['reset']) {
        $data['reset'] = true;
      }
      else {
        unset($data['reset']);
      }
      return $data;
    break;
    
    // Cleanup any global settings after the action runs
    case 'cleanup':
      unset($_POST['op']);
    break;
  }
}