<?php

function views_patterns($op, $id = null, &$data = null, $a = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('view');
    break;
    
    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(

      );
    break;
    
    // Return a summary of an action
    case 'summary':

    break;
    
    // Prepare data for views-centric structure
    case 'prepare':

    break;
    
    // Validate and cleanup the values for an action before running the pattern
    case 'pre-validate':

    break;
    
    // Ask the user if its ok to do certain tasks
    case 'confirmation form':
    break;
    
    // Validate action before processing
    case 'validate':

    break;
    
    // Return the form id or determine if the action does not need processing
    case 'form_id':

    break;
    
    // Add default values to the pattern where appropriate and return form data 
    case 'build':

      return $data;
    break;
    
    // Create extra parameters needed for a pattern execution
    case 'params':

    break;
   
    // Return the default values for an action
    case 'defaults':

    break;
  }
}

function views_patterns_mappings($type) {
  switch($type) {

  }
  
  return $mappings;
}
