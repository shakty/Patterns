<?php

function field_patterns () {

  $files = array('modules/field_ui/field_ui.admin.inc');
  $actions['field'] = array('descr' => t('Create/Modify/Delete field'), 
                                 PATTERNS_MODIFY => array('field_ui_field_settings_form'),
                                 PATTERNS_FILES => $files,
                                );
  $actions['instance'] = array('descr' => t('Create/Modify/Delete field'), 
                                 PATTERNS_CREATE => array('field_ui_field_overview_form'),
                                 PATTERNS_MODIFY => array('field_ui_field_edit_form'),
                                 PATTERNS_DELETE => array('field_ui_field_delete_form'),
                                 PATTERNS_FILES  => $files,
                                );
  $actions['field_weight'] = array('descr' => t('Order the fields'), 
                                 PATTERNS_MODIFY => array('field_ui_field_overview_form'),
                                 PATTERNS_FILES  => $files,
                                );
                          
  return $actions;                
  
}

function field_patterns_actions() {
   return array(
        'field_ui_field_overview_form' => t('Add/Create a Field'),
        'field_ui_field_delete_form' => t('Delete a Field/Istance'),
        'field_ui_field_edit_form' => t('Define extra settings for Instances'),
        'field_ui_field_settings_form' => t('Define extra settings for Field')
    );
}

// Prepare data for processing
function field_patterns_prepare($action, $tag, &$data = NULL) {
	 // @TODO: remove unnecessary stuff from $data array;
      if (isset($data['name'])) {
        $data['field_name']='field_' . $data['name'];
      }
      if (isset($data['entity'])) {
        $data['entity_type']=$data['entity'];
      }   
        // FIELD
      if ($tag == 'instance') {       
          // DELETE
          if ($action == PATTERNS_DELETE) {
            $data['confirm'] =  1;
            $data['op'] = 'Delete';        
          }          
          // CREATE/MODIFY
          else {
          	$data = _field_patterns_prepare_instance($action, $data);
          }
      }// End: $id = field
      elseif($tag == 'field'){
        $data = _field_patterns_prepare_field($action, $data);
      }else{
      	foreach($data['weight'] as $key=>$lev){
      		$name='field_'.$key;
      		$data['fields'][$name]['weight']=$lev;
      	}
      	unset($data['weight']);
      }
      
  return patterns_results();
}

// Validate the values for an action before running the pattern
function field_patterns_validate ($action, $tag, &$data = NULL) {
	$status = PATTERNS_SUCCESS;
  $msg = '';
	if($tag=='field_weight'){
		if(!isset($data['entity_type'])){
      $status = PATTERNS_ERR;
      $msg = t('FieldException: Field entity type is required.');
    }
    elseif(!isset($data['bundle'])){
      $status = PATTERNS_ERR;
      $msg = t('FieldException: Field bundle is required.');
    }
	}
	if($tag=='field'){
		if(!isset($data['field_name'])){
    	$status = PATTERNS_ERR;
    	$msg = t('FieldException: Field name is required.');
  	}
	}	
  if($tag=='instance'){
		$exist=db_query("SELECT COUNT(*) FROM {field_config_instance} WHERE field_name = :name and entity_type = :type and bundle = :bundle", array('bundle'=>$data['bundle'],'type'=>$data['entity_type'],'name'=>$data['field_name']))->fetchField();
		if(!isset($data['field_name'])){
    	$status = PATTERNS_ERR;
    	$msg = t('FieldException: Field name is required.');
  	}
  	elseif(!isset($data['entity_type'])){
      $status = PATTERNS_ERR;
      $msg = t('FieldException: Field entity type is required.');
    }
    elseif(!isset($data['bundle'])){
      $status = PATTERNS_ERR;
      $msg = t('FieldException: Field bundle is required.');
    }
    elseif($action==PATTERNS_CREATE && $exist){
      $status = PATTERNS_ERR;
      $msg = t('Add new field: the field name %name already exists.', array('%name' => $data['field_name']));
    }elseif($action==PATTERNS_MODIFY && !$exist){
      $status = PATTERNS_ERR;
      $msg = t('FieldException: Attempt to update an instance of a nonexistent field.');
    }
    elseif($action==PATTERNS_DELETE && !$exist){
      $status = PATTERNS_ERR;
      $msg = t('FieldException: Attempt to delete an instance of a nonexistent field');
    }
  }       
 
  return patterns_results($status, $msg);          
	
}

// Return which callback functions to actually use.
function field_patterns_callbacks ($action, $tag, &$data = NULL) {
  $desc = field_patterns();
  $result = $desc[$tag][$action];
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Prepare for valid processing of this type of component
function field_patterns_build ($action, $tag, &$data = NULL, $form_id) {
   //content_clear_type_cache();
  module_load_include('inc', 'field_ui', 'field_ui.admin');
  return patterns_results(PATTERNS_SUCCESS,  t('Execution successful'), $data);
}

// Build a patterns actions and parameters
function field_patterns_params($action, $form_id, &$data = NULL) {
   if ($form_id == 'field_ui_field_overview_form') {
      $result = array($data['entity_type'],$data['bundle']);
   }
   else if ($form_id == 'field_ui_field_edit_form' ) {
   	$instance = field_info_instance($data['entity_type'], $data['field_name'], $data['bundle']);   	
    $result=array($instance);   
   }
   else if ( $form_id == 'field_ui_field_delete_form') {
        
        /*
         * delete all the field instance with the field_name 
         * $fname = $data['field_name'];
        $instance = db_query("SELECT * FROM {field_config_instance} WHERE field_name = :fname", array('fname' => $fname))->fetchAssoc();
        */    
      	$result=array(array('bundle'=>$data['bundle'],'entity_type'=>$data['entity_type'],'field_name'=>$data['field_name']));   
   }elseif($form_id=='field_ui_field_settings_form'){
      $result=array(array('bundle'=>$data['bundle'],'entity_type'=>$data['entity_type'],'field_name'=>$data['field_name']));   
   }  
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

function field_patterns_cleanup($action, $tag, &$data = NULL) {
 return patterns_results();
}

function _field_patterns_prepare_instance($action, $data){
	$PREFIX      = 'field_';
  $EXISTING_F  = '_add_existing_field';
  $NEW_F       = '_add_new_field';
  
  $all_fields = field_info_fields();  
  $EXISTING= array_key_exists($PREFIX.$data['name'],$all_fields)?$EXISTING_F:$NEW_F;  
	$data['fields'] = array(); 
	$fname = $PREFIX . $data['name'];
	
	// Setting default values which can be overriden later
            
  $default_type = 'text'; //text_textfield';
 // $default_parent = '';
  $default_weight = 1;
  $default_widget_type = 'text_textfield';
  $default_cardinality = 1;
  $default_label = $fname;
  if($action==PATTERNS_CREATE){
              // NAME
    $data['fields'][$EXISTING]['field_name'] = $fname;
    unset($data['name']);           
            // LABEL
    if (isset($data['label'])) {
        $data['fields'][$EXISTING]['label'] = $data['label'];
        unset($data['label']);
    }
    else {
        $data['fields'][$EXISTING]['label'] = $default_label;
    }
           
            // WEIGHT
    if (isset($data['weight'])) {
        $data['fields'][$EXISTING]['weight'] = $data['weight'];
        unset($data['weight']);
    }
    else {  
       $data['fields'][$EXISTING]['weight'] = $default_weight;
    }
            
            // PARENT(unused parameter?)
    /*if (isset($data['parent'])) {
       $data['fields'][$EXISTING]['parent'] = $data['parent'];
       unset($data['parent']);
    }
            else {  
              $data['fields'][$EXISTING]['parent'] = $default_parent;
            }*/
            
            // TYPE
    if (isset($data['type'])) {
       $data['fields'][$EXISTING]['type'] = $data['type']; 
       unset($data['type']);
    }
    else {
       $data['fields'][$EXISTING]['type'] = $default_type;
    }
            
            // WIDGET
            
            // @TODO: check for incompatible widget_type and field_type
            // @TODO: add aliases for long names
            
            // Term Reference:
            // taxonomy_autocomplete
            // options_buttons
    if (isset($data['widget'])) {
              
       switch ($data['widget']) {
    
           case 'autocomplete':
                  $w = 'taxonomy_autocomplete';
                  break;
           case 'buttons':
                  $w = 'options_buttons';
                  break;
           default:
                  $w = $data['widget'];
       }        
       $data['fields'][$EXISTING]['widget_type'] = $w;
       unset($data['widget']);
    }
    else {
       $data['fields'][$EXISTING]['widget_type'] =  $default_widget_type;
    } 
  }
// INSTANCE
  elseif($action==PATTERNS_MODIFY){
  	// INSTANCE
    if(isset($data['label'])){
       $data['instance']['label'] = $data['label'];
    }
    // REQUIRED
    if (isset($data['required']) && $data['required']) {
        $data['instance']['required'] = 1;
        unset($data['required']);
    }
    // HELP DESCR
    if (isset($data['description'])) {
        $data['instance']['description'] = $data['description'];
        unset($data['description']);
    }
     // CARDINALITY
    if (isset($data['cardinality'])) {
       //$c = array('und' => array(array('value' =>$data['cardinality'],'safe_value'=>$data['cardinality'])));
       $data['field']['cardinality'] = $data['cardinality'];
       unset($data['cardinality']);
    }
    else {
       $data['field']['cardinality'] = $default_cardinality;
    }
    if (isset($data['text_processing'])) {
       //$c = array('und' => array(array('value' =>$data['cardinality'],'safe_value'=>$data['cardinality'])));
       $data['instance']['settings']['text_processing'] = $data['text_processing'];
       unset($data['text_processing']);
    }
    if (isset($data['max_length'])) {
       //$c = array('und' => array(array('value' =>$data['cardinality'],'safe_value'=>$data['cardinality'])));
       $data['field']['settings']['max_length'] = $data['max_length'];
       unset($data['max_length']);
    }
    if (isset($data['summary'])) {
       //$c = array('und' => array(array('value' =>$data['cardinality'],'safe_value'=>$data['cardinality'])));
       $data['instance']['settings']['display_summary'] = $data['summary'];
       unset($data['summary']);
    }
    if (isset($data['rows'])) {
       //$c = array('und' => array(array('value' =>$data['cardinality'],'safe_value'=>$data['cardinality'])));
       $data['instance']['widget']['settings']['rows'] = $data['rows'];
       $date['instance']['widget']['active']=1;
       unset($data['rows']);
    }
    if (isset($data['size'])) {
       //$c = array('und' => array(array('value' =>$data['cardinality'],'safe_value'=>$data['cardinality'])));
       $data['instance']['widget']['settings']['size'] = $data['size'];
       $date['instance']['widget']['active']=1;
       unset($data['size']);
    }
    //default_value
     if (isset($data['default_value'])) {
       $data[$fname]['und'][0]['value'] = $data['default_value'];
       unset($data['default_value']);
    }
   
    //other special field type settings form
    if(isset($data['instance_settings'])){
    	foreach($data['special_settings'] as $key=>$setting){
    		$data['instance']['setting'][$key]=$setting;
    	}
    	unset($data['settings']);
    }
    if(isset($data['field_settings'])){
      foreach($data['field_settings'] as $key=>$setting){
        $data['field']['setting'][$key]=$setting;
      }
      unset($data['settings']);
    }
    if(isset($data['widget_settings'])){
      foreach($data['widget_settings'] as $key=>$setting){
        $data['instance']['widget']['setting'][$key]=$setting;
      }
      $date['instance']['widget']['active']=1;
      unset($data['settings']);
    }         
  }
  return $data;        
}

 function _field_patterns_prepare_field($action, $data){
 	$data['field'] = array();
 	foreach($data['settings'] as $key=>$setting){
 		$data['field']['settings'][$key]=$setting;
 	}
 	unset($data['settings']);
 	return $data;
 }

function field_patterns_old($op, $id = NULL, &$data = NULL, $a = NULL) {
	
  switch ($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('field','instance'); // instance not used now
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'field_ui_field_overview_form' => t('Add/Create a Field'),
        'field_ui_field_delete_form' => t('Delete a Field/Istance'),
        //'field_ui_field_edit_form' => t('Define extra settings for Instances') // to edit a field already created
        'field_ui_field_settings_form' => t('Define extra settings for Instances'),
      );
    break;

    // Prepare the data
    case 'prepare':
      
      // @TODO: remove unnecessary stuff from $data array;
      
      

      if (isset($data['name'])) {
  
        // FIELD
        if ($id == 'field') {
          
          // DELETE
          if (isset($data['delete'])) {
            
            if (isset($data['name'])) {
              $data['field_name'] = $data['name'];
              unset($data['name']); 
            }
             
            if (isset($data['entity'])) {
              $data['entity_type'] = $data['entity'];
              unset($data['entity']); 
            }
            
            // not relevant
//            if (isset($data['bundle'])) {
//              $data['bundle'] = $data['bundle'];
//            }
  
            $data['confirm'] =  1;
            $data['op'] = 'Delete';

        
          }// End: delete
          
          // CREATE
          else {
                 
            $data['fields'] = array();
        
            // Is a new field?
            $all_fields = field_info_fields();     
            $EXISTING = (array_key_exists($PREFIX.$data['name'],$all_fields)) ? $EXISTING_F : $NEW_F;
            
            // NAME
            $fname = $PREFIX . $data['name'];
            $data['fields'][$EXISTING]['field_name'] = $fname;
            unset($data['name']);
            
  	        // Setting default values which can be overriden later
  	        
  	        $default_type = 'text'; //text_textfield';
  	        $default_parent = '';
  	        $default_weight = 1;
  	        $default_widget_type = 'text_textfield';
  	        $default_cardinality = 1;
  	        $default_label = $fname;
  	        
  	        // Not needed anymore?
  	        $default_storage_type = 'field_sql_storage';
  
  	        // LABEL
  	        if (isset($data['label'])) {
  	          $data['fields'][$EXISTING]['label'] = $data['label'];
  	          unset($data['label']);
  	        }
  	        else {
  	          $data['fields'][$EXISTING]['label'] = $default_label;
  	        }
  	       
  	        // WEIGHT
  	        if (isset($data['weight'])) {
              $data['fields'][$EXISTING]['weight'] = $data['weight'];
              unset($data['weight']);
            }
            else {  
              $data['fields'][$EXISTING]['weight'] = $default_weight;
            }
            
            // PARENT
            if (isset($data['parent'])) {
              $data['fields'][$EXISTING]['parent'] = $data['parent'];
              unset($data['parent']);
            }
            else {  
              $data['fields'][$EXISTING]['parent'] = $default_parent;
            }
            
  	        // TYPE
  	        if (isset($data['type'])) {
  	          $data['fields'][$EXISTING]['type'] = $data['type']; 
  	          unset($data['type']);
  	        }
  	        else {
  	          $data['fields'][$EXISTING]['type'] = $default_type;
  	        }
  	        
  	        // WIDGET
  	        
  	        // @TODO: check for incompatible widget_type and field_type
  	        // @TODO: add aliases for long names
  	        
  	        // Term Reference:
  	        // taxonomy_autocomplete
  	        // options_buttons
  	        if (isset($data['widget'])) {
  	          
  	          switch ($data['widget']) {
  	
  	            case 'autocomplete':
  	              $w = 'taxonomy_autocomplete';
  	              break;
  	            case 'buttons':
  	              $w = 'options_buttons';
  	              break;
  	            default:
  	              $w = $data['widget'];
  	          }        
  	          $data['fields'][$EXISTING]['widget_type'] = $w;
  	          unset($data['widget']);
  	        }
  	        else {
  	          $data['fields'][$EXISTING]['widget_type'] =  $default_widget_type;
  	        }
  	        
  	        
  	        // FIELD-SPECIFIC OPTIONS
  	        switch ($data['fields'][$EXISTING]['type']) {
  	          
  	          case 'taxonomy_term_reference':
  	            if (isset($data['vocabulary'])) {
  	              $data['field']['settings']['allowed_values'][0]['vocabulary'] = $data['vocabulary'];
  	              unset($data['vocabulary']);
  	            }
  	            else {
  	              //@TODO If not vocabulary selected pick up the first one
  	            }
  	        
  	        }  
  	        
  	        // INSTANCE
  	        
  	        
  	        // CARDINALITY
  	        if (isset($data['cardinality'])) {
  	           //$c = array('und' => array(array('value' =>$data['cardinality'],'safe_value'=>$data['cardinality'])));
  	           $data['field']['cardinality'] = $data['cardinality'];
  	           unset($data['cardinality']);
  	        }
  	        else {
  	          $data['field']['cardinality'] = $default_cardinality;
  	        }
  	                
  	        // REQUIRED
  	        if (isset($data['required']) && $data['required']) {
  	          $data['instance']['required'] = 1;
  	          unset($data['required']);
  	        }
  	        
  	        // PRESENTED AT REGISTRATION TIME
  	        if (isset($data['registration']) && $data['registration']) {
  	          $data['instance']['settings']['user_register_form'] = 1;
  	          unset($data['registration']);
  	        }
  	        
  	        // STORAGE ENGINE
  	        if (isset($data['storage'])) {
  	          $data['instance']['settings']['storage']['type'] = $data['storage'];
  	          unset($data['storage']);
  	        }
  	        else {
  	          $data['instance']['settings']['storage']['type'] = $default_storage_type;
  	        }
  	        
  	        if (isset($data['default'])) {
  	          $data[$fname]['und'] = $data['default'];
  	          unset($data['default']); 
  	        }
          	
            // INSTANCE
            
            $data['instance']['label'] = $data['fields'][$EXISTING]['label'];
            $data['instance']['field_name'] = $fname;  
            
            if (isset($data['entity'])) {
              $data['instance']['entity_type'] = $data['entity'];
              unset($data['entity']); 
            }
            
            if (isset($data['bundle'])) {
              $data['instance']['bundle'] = $data['bundle'];
              unset($data['bundle']); 
            }
  
            // HELP DESCR
            if (isset($data['description'])) {
              $data['instance']['description'] = $data['description'];
              unset($data['description']);
            }    
  
            // WIDGET
            if (isset($data['description'])) {
              $data['instance']['description'] = $data['fields'][$EXISTING]['weight'];
              unset($data['description']);
            }   
            
            $data['instance']['widget']['weight'] = $data['fields'][$EXISTING]['weight'];
            $data['instance']['widget']['type'] = $data['fields'][$EXISTING]['widget_type'];
            
            $data['instance']['widget']['module'] = 'taxonomy';
            $data['instance']['widget']['active'] = 0;
            
            $data['instance']['display']['weight'] = 45;
            $data['instance']['display']['type'] = 'taxonomy_term_reference_link';
            $data['instance']['display']['module'] = 'taxonomy';
            $data['instance']['display']['label'] = 'above';
            $data['instance']['display']['settings'] = array();
  
            $data['instance']['default_value_widget']  = 'user';
            
      
          }// End: create field/instance
        }// End: $id = field
      }// End: case = prepare
    

    break;

    // Validate the values for an action before running the pattern
    case 'pre-validate':
//     
//     if ($id == 'field') {
//        if (empty($data['type_name'])) {
//          return t('&lt;type&gt; tag is missing.');
//        }
//        if (empty($data['field_name'])) {
//          return t('&lt;name&gt; tag is missing.');
//        }
//      }
  
    break;

    // Validate action before processing
    case 'validate':

       // nothing
        
    break;

    // Return the form id or determine if the action does not need processing
    case 'form_id':
      if ($id == 'field') {
  
        if (isset($data['delete'])) {
          return array('field_ui_field_delete_form');
        }
        else {
          return array(
                      'field_ui_field_overview_form',
                      //'field_ui_field_edit_form'
                      'field_ui_field_settings_form',
                     );  
        }
        
      }
      
    break;

    // Add default values to the pattern where appropriate and return form data
    case 'build':
      //content_clear_type_cache();
      module_load_include('inc', 'field_ui', 'field_ui.admin');
          
      return $data;
      
    break;

    // Create extra parameters needed for a pattern execution
    case 'params':
	
      if ($id == 'field_ui_field_overview_form') {
      	$entity = $data['instance']['entity_type'];
        $bundle = $data['instance']['bundle'];
      
        return array($entity,$bundle);
      }
      //else if ($id == 'field_ui_field_edit_form' ) {
      else if ($id == 'field_ui_field_settings_form' ) {
        
        // Was
      	//$fname = $PREFIX . $data['fields'][$EXISTING]['field_name'];
        $fname = $data['fields'][$EXISTING]['field_name'];
      	$instance = db_query("SELECT * FROM {field_config_instance} WHERE field_name = :fname", array('fname' => $fname))->fetchAssoc();
        return array($instance);
      }
      
      else if ( $id == 'field_ui_field_delete_form') {
        
        $fname = $data['field_name'];
        $instance = db_query("SELECT * FROM {field_config_instance} WHERE field_name = :fname", array('fname' => $fname))->fetchAssoc();
        return array($instance);
      }
      
      
    break;

    // Any last cleanup items
    case 'cleanup':
      // Make sure content info is always up-to-date
//      cache_clear_all('content_type_info', content_cache_tablename());
//      _content_type_info(true);
//
//      // make sure fieldgroup info is up to date
//      cache_clear_all('fieldgroup_data', content_cache_tablename());
//      if (module_exists('fieldgroup')) {
//        fieldgroup_groups('', FALSE, TRUE);
//      }
//      // filter out confusing warning about 'allowed values' produced by optionwidgets module
//      if ($id == 'content_field_edit_form' && strpos($data['widget_type'], 'optionwidget') === 0 && $warnings = drupal_get_messages('warning')) {
//        foreach ($warnings['warning'] as $warning) {
//          if ($warning != "You need to specify the 'allowed values' for this field.") {
//            drupal_set_message($warning, 'warning');
//          }
//        }
//      }
    break;
  }
  
}



//$macro[0]['form_id'] = 'field_ui_field_overview_form';
//$macro[0]['path'] = 'admin/config/people/accounts/fields';
//$macro[0]['values']  = array (
//  'fields' => 
//  array (
//    'field_name_first' => 
//    array (
//      'weight' => '1',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => 'field_name_first',
//      ),
//    ),
//    'field_skills' => 
//    array (
//      'weight' => '-10',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => 'field_skills',
//      ),
//    ),
//    'field_name_first2' => 
//    array (
//      'weight' => '1',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => 'field_name_first2',
//      ),
//    ),
//    'field_sponsor' => 
//    array (
//      'weight' => '1',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => 'field_sponsor',
//      ),
//    ),
//    'field_sponsor6' => 
//    array (
//      'weight' => '1',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => 'field_sponsor6',
//      ),
//    ),
//    'field_sponsor7' => 
//    array (
//      'weight' => '1',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => 'field_sponsor7',
//      ),
//    ),
//    'field_sponsor9' => 
//    array (
//      'weight' => '1',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => 'field_sponsor9',
//      ),
//    ),
//    'field_sponsor5' => 
//    array (
//      'weight' => '8',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => 'field_sponsor5',
//      ),
//    ),
//    'account' => 
//    array (
//      'weight' => '-10',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => 'account',
//      ),
//    ),
//    'timezone' => 
//    array (
//      'weight' => '6',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => 'timezone',
//      ),
//    ),
//    '_add_new_field' => 
//    array (
//      'label' => '',
//      'weight' => '9',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => '_add_new_field',
//      ),
//      'field_name' => '',
//      'type' => '',
//      'widget_type' => '',
//      'translatable' => false,
//    ),
//    '_add_existing_field' => 
//    array (
//      'label' => 'Ste',
//      'weight' => '10',
//      'parent' => '',
//      'parent_wrapper' => 
//      array (
//        'hidden_name' => '_add_existing_field',
//      ),
//      'field_name' => 'field_image',
//      'widget_type' => 'image_image',
//    ),
//  ),
//  'op' => 'Save',
//);
//$macro[0]['parameters']  = 'N;';
//
//$macro[1]['form_id'] = 'field_ui_field_edit_form';
//$macro[1]['path'] = 'admin/config/people/accounts/fields/field_image/edit';
//$macro[1]['values']  = array (
//  'instance' => 
//  array (
//    'field_name' => 'field_image',
//    'entity_type' => 'user',
//    'bundle' => 'user',
//    'widget' => 
//    array (
//      'weight' => '10',
//      'type' => 'image_image',
//      'module' => 'image',
//      'active' => 1,
//      'settings' => 
//      array (
//        'progress_indicator' => 'throbber',
//        'preview_image_style' => 'thumbnail',
//      ),
//    ),
//    'label' => 'Ste',
//    'required' => 0,
//    'description' => 'afa',
//    'settings' => 
//    array (
//      'file_directory' => '',
//      'file_extensions' => 'png gif jpg jpeg',
//      'max_filesize' => '8mb',
//      'max_resolution' => '200x200',
//      'min_resolution' => '',
//      'alt_field' => 1,
//      'title_field' => 1,
//      'user_register_form' => 1,
//    ),
//  ),
//  'field' => 
//  array (
//    'cardinality' => '1',
//    'settings' => 
//    array (
//      'uri_scheme' => 'public',
//      'default_image' => 0,
//    ),
//  ),
//  'op' => 'Save settings',
//);
//$macro[1]['parameters']  = 'N;';


    /*
    $instance Array [8] 
    [0...7] 
    field_name  field_urcaurcatirulero18  
    entity_type user  
    bundle  user  
    widget  Array [4] 
    label urcaurcatirulero17  
    required  1 
    description uippi 
    settings  Array [1] 
      [0...0] 
        user_register_form  1 
        
        
        $field  Array [2] 
  [0...1] 
    cardinality 4 
    settings  Array [1] 
      [0...0] 
        allowed_values  Array [1] 
          [0...0] 
            0 Array [2] 
              [0...1] 
                vocabulary  anothervoc  
                parent  0 

        
        
        */
        
            /*
       * 
  
  
  values  Array [8] 
  [0...7] 
    instance  Array [8] 
      [0...7] 
        field_name  field_urcaurcatirulero14  
        entity_type user  
        bundle  user  
        widget  Array [4] 
        label urcaurcatirulero14  
        required  1 
        description dddddddddddddddd  
        settings  Array [1] 
          [0...0] 
            user_register_form  1 
    field_urcaurcatirulero14  Array [1] 
    field Array [2] 
      [0...1] 
        cardinality 9 
        settings  Array [1] 
          [0...0] 
            allowed_values  Array [1] 
              [0...0] 
                0 Array [2] 
                  [0...1] 
                    vocabulary  anothervoc  
                    parent  0 
    submit  Save settings 
    form_build_id form-QanhE_ceAaqd7KgYRhTz3_r3fIhcl5h7n4lva4cRcOY  
    form_token  sYPnxGLjTu_gPgeIzwNNofahRa9oNCJV-DpWQU2stYc 
    form_id field_ui_field_edit_form  
    op  Save settings 

  
       * 
       * 
       * 
       */


  /*      
      
 input  Array [7] 
  [0...6] 

    fields  Array [1] 
      [0...0] 
        _add_new_field  Array [6] 
          [0...5] 
            label urcaurcatirulero15  
            field_name  urcaurcatirulero15  
            type  taxonomy_term_reference 
            widget_type taxonomy_autocomplete 
            weight  -10 
            parent  
            
            
            
    field Array [2] 
      [0...1] 
        settings  Array [1] 
          [0...0] 
            allowed_values  Array [1] 
        cardinality 5 
        
        
    instance  Array [3] 
      [0...2] 
       ----- required  1 
        settings  Array [2] 
          [0...1] 
            user_register_form  1 
            storage Array [1] 
      -------  description Stefano is cool 
    
    
    field_urcaurcatirulero15  Array [1] 
      [0...0] 
        und Economics 


input Array [7] 
  [0...6] 
    instance  Array [4] 
      [0...3] 
        label urcaurcatirulero13  *********
        required  1 
        description dede  
        settings  Array [1] 
        
    field_urcaurcatirulero13  Array [1] 
      [0...0] 
        und Economics 
        
        
    field Array [2] 
      [0...1] 
        cardinality 6 
        settings  Array [1] 
          [0...0] 
            allowed_values  Array [1] 
              [0...0] 
                0 Array [1] 
                  [0...0] 
                    vocabulary  anothervoc  
   
      
      field Array [1] 
  [0...0] 
    field_urcaurcatirulero12  Array [1] 
      [0...0] 
        und Array [5] 
          [0...4] 
            field Array [16]  
              [0...15]  
                translatable  1 
                entity_types  Array [0] 
                settings  Array [1] 
                storage Array [5] 
                foreign keys  Array [1] 
                indexes Array [1] 
                id  104 
                field_name  field_urcaurcatirulero12  
                type  taxonomy_term_reference 
                module  taxonomy  
                active  1 
                locked  0 
                cardinality 10  
                deleted 0 
                columns Array [1] 
                bundles Array [1] 
            instance  Array [13]  
              [0...12]  
            items_count 1 
            array_parents Array [4] 
            errors  Array [0] 
      
      */
      
//      $form_state Array [13]  
//  [0...12]  
//    build_info  Array [2] 
//      [0...1] 
//        args  Array [2] 
//          [0...1] 
//            0 user  
//            1 user  
//        files Array [1] 
//          [0...0] 
//            menu  modules/field_ui/field_ui.admin.inc 
//    rebuild false 
//    rebuild_info  Array [0] 
//    redirect  <Uninitialized> 
//    temporary Array [0] 
//    submitted false 
//    executed  false 
//    programmed  false 
//    cache false 
//    method  post  
//    groups  Array [0] 
//    buttons Array [0] 
//    input Array [5] 
//      [0...4] 
//        fields  Array [10]  
//          [0...9] 
//            account Array [3] 
//            field_ullalah Array [3] 
//            field_stefanoiscool Array [3] 
//            timezone  Array [3] 
//            profyle_this_is_new_field Array [3] 
//            field_sterel  Array [3] 
//            field_science_discipline  Array [3] 
//            profyle_  Array [3] 
//            _add_new_field  Array [7] 
//              [0...6] 
//                label proviamounnuovo 
//                weight  1 
//                parent  
//                parent_wrapper  Array [1] 
//                field_name  prunnuo 
//                type  text  
//                widget_type text_textfield  
//            _add_existing_field Array [5] 
//        form_build_id form-2xW-DQZZDFiX8z0qTGxZm9u4iiy4D_W6rU5allBIhyo  
//        form_token  DgxOX-XuYgd2-EYSMVGmKtHTOkwrakqnYOK1mmhUHCs 
//        form_id field_ui_field_overview_form  
//        op  Save  
