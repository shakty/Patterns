<?php
/*
 * @file
 * Patterns component for date.
 */
 function date_patterns() {
	$files = array('sites/all/modules/date/date_admin.inc');
  	$actions['date'] = array('descr' => t('Create/edit/delete types of date and time'),
  								 PATTERNS_CREATE => array('system_configure_date_formats_form','system_add_date_format_type_form'), 
                                 PATTERNS_MODIFY => array('system_configure_date_formats_form','system_date_time_settings'),
                                 PATTERNS_DELETE => array('system_delete_date_format_type_form'),
                                 PATTERNS_FILES =>$files
                                );
                                                  
  return $actions;                
  
}

function blog_patterns_actions() {
  return array(
        'system_add_date_format_type_form' => t('Create a type of date and time'),
        'system_date_time_settings' => t('Edit the types of date and time'),
        'system_delete_date_format_type_form' => t('Delete a type of date and time'),
        'system_configure_date_formats_form' => t('Create/edit formats of date and time'),
      );
}

// Prepare data for processing
function blog_patterns_prepare($action, $tag, &$data = NULL) {
		if(isset($data['format'])){
			$data['date_format']=$data['format'];
			unset($data['format']);
		}
		if(isset($data['type'])){
			if(!isset($data['machine_name'])){
				$data['machine_name']=str_replace(' ', '-', $data['type']);
			}
			$data['date_type']=$data['type'];
			unset($data['type']);
		}
	if($action==PATTERNS_MODIFY){
		if(isset($data['date_type'])&&isset($data['date_format'])){
			$new='date_format_'.$data['date_type'];
			$data[$new]=$data['date_format'];
		}
	}
    return patterns_results();
}

// Validate the values for an action before running the pattern
function blog_patterns_validate ($action, $tag, &$data = NULL) {
	$status = PATTERNS_SUCCESS;
  	$msg = ''; 
    return patterns_results($status, $msg);    
}

// Prepare for valid processing of this type of component
function blog_patterns_build ($action, $tag, &$data = NULL, $form_id) {
	 module_load_include('inc', 'system', 'system.admin');
  return patterns_results(PATTERNS_SUCCESS,  t('Execution successful'), $data);
}

// Build a patterns actions and parameters
function blog_patterns_params($action, $form_id, &$data = NULL,&$a) {
	if($action==PATTERNS_DELETE){
		$data['confirm']=1;
		$result=array($data['date_type']);
	}

    return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

function blog_patterns_cleanup($action, $tag, &$data = NULL) {
 return patterns_results();
}

// Return which callback functions to actually use.
function blog_patterns_callbacks ($action, $tag, &$data = NULL) {
	$desc = blog_patterns();
	if($action==PATTERNS_CREATE || $action==PATTERNS_MODIFY){
		$dfid=db_query("SELECT dfid FROM {date_formats} WHERE format = :format AND locked = 0", array('format' => $data['date_format']))->fetchField();
		if($dfid){
			$result = array($desc[$tag][$action][1]);
		}else{
			$result = $desc[$tag][$action];
		}
	}else{
  		$result = $desc[$tag][$action];
	}
  	return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}
