<?php

function content_patterns($op, $id = null, &$data = null, $a = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('content', 'field', 'display', 'group');
    break;
    
    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'node_type_form' => t('Create Content Type'),
        'node_type_delete_confirm' => t('Delete Content Type'),
        'node_type_form' => t('Update Content Type'),
        '_content_admin_field_add_new' => t('Add Fieldtype'),
        '_content_admin_field_remove' => t('Delete Fieldtype'),
        '_content_admin_field' => t('Update Fieldtype'),
        '_content_admin_field_add_existing' => t('Add Existing Fieldtype'),
        'content_admin_display_overview_form'=> t('Content fields display'),
        'fieldgroup_edit_group_form' => t('Add/edit a fieldgroup')
      );
    break;
    
    // Prepare the data
    case 'prepare':
      if ($id == 'field') {
        if ($data['name'] && !$data['field_name']) {
          $data['field_name'] = 'field_'. $data['name'];
        }
        
        if ($data['group'] && strpos('group_', $data['group']) !== 0) {
          $data['group'] = 'group_'. $data['group'];
        }
      }
      else if ($id == 'display') {
        $type = $data['type'];
        $field = $data['field'];
        unset($data['type'], $data['field']);
        
        if (strpos($field, 'field_') !== 0) {
          $field = 'field_'. $field;
        }
        
        $data = array('fields' => array($field => $data), 'type_name' => $type);
        
        foreach($data['fields'][$field] as $key => $value) {
          $data['fields'][$field][$key] = array('format' => $value);
        }
      }
      else if ($id == 'group') {
        if ($data['name']) {
          $data['group_name'] = $data['name'];
          unset($data['name']);
        }
        
        if (!$data['group_name'] && $data['label']) {
          $data['group_name'] = strtolower(str_replace(array(' ', '-'), '_', $data['label']));
        }
        
        if ($data['group_name'] && strpos('group_', $data['group_name']) !== 0) {
          $data['group_name'] = 'group_'. $data['group_name'];
        }
        
        $fields = array('type', 'label', 'weight', 'group_name', 'settings');
        foreach($data as $key => $value) {
          if (!in_array($key, $fields)) {
            $data['settings'][$key] = $value;
            unset($data[$key]);
          }
        }
      }
    break;
    
    // Validate the values for an action before running the pattern
    case 'pre-validate':
      if ($id == 'content') {
        $type = $data['type'];
        $name = $data['name'];
        
        if (!$type) {
          return t('Missing tag &lt;type&gt; inside of &lt;content&gt;');
        }
      }
    break;
    
    // Ask the user if its ok to do certain tasks
    case 'confirmation form':
      if ($id == 'content') {
        if ($data['delete'] && content_types($data['type'])) {
          return array('message' => array(
            '#type' => 'markup',
            '#value' => t('Is it ok to delete %type content type?', array('%type' => $data['type']))
          ));
        }
      }
    break;
    
    // Validate action before processing
    case 'validate':
    break;
    
    // Return the form id or determine if the action does not need processing
    case 'form_id':
      if ($id == 'content') {
        if ($data['delete']) {
          if (!content_types($data['type'])) {
            return;
          }

          return 'node_type_delete_confirm';
        }
        else {
          return 'node_type_form';
        }
      }
      else if ($id == 'field') {
        // Make sure content type info is up-to-date
        _content_type_info(true);
        node_get_types('types', NULL, true);
        
        if ($data['delete']) {
          return '_content_admin_field_remove';
        }
        else {
          $field = content_fields($data['field_name'], $data['type']);
          if ($field) {
            if ($data['delete']) {
              return '_content_admin_field_remove';
            }
            else {
              return '_content_admin_field';
            }
          }
          else {
            $field = content_fields($data['field_name']);
            
            if ($data['delete'] && $data['type']) {
              return;
            }
            else if ($data['delete']) {
              return '_content_admin_field_remove';
            }
            
            if ($field) {
              return array('_content_admin_field_add_existing', '_content_admin_field');
            }
            else {
              return array('_content_admin_field_add_new', '_content_admin_field');
            }
          }
        }
      }
      else if ($id == 'display') {
        return 'content_admin_display_overview_form';
      }
      else if ($id == 'group') {
        return 'fieldgroup_edit_group_form';
      }
    break;
    
    // Add default values to the pattern where appropriate and return form data 
    case 'build':
      content_clear_type_cache();
      require_once drupal_get_path('module', 'content') .'/content_admin.inc';
      require_once drupal_get_path('module', 'node') .'/content_types.inc';
      
      $type = $data['type'];
      $name = $data['name'];
      $types = content_types($type);
      
      if ($id == 'node_type_form') {
        if (!$types) {
          // Set the name of the new content type if missing
          if (!$data['name']) {
            $data['name'] = $type;
          }
          
          $defaults = _node_type_set_defaults($data);
          
          foreach($defaults as $key => $value) {
            if (!isset($data[$key])) {
              $data[$key] = $value;
            }
          }
        }
        
        $node_options = array('status', 'published', 'promote', 'moderate');
        $default_options = variable_get('node_options_'. $type, null);
        
        if (!empty($default_options)) {
          $data['node_options'] = array_combine($default_options, $default_options);
        }
        
        foreach($data as $key => $value) {
          if (in_array($key, $node_options)) {
            if ($value) {
              $data['node_options'][$key] = $key;
            }
            else {
              $data['node_options'][$key] = '';
            }
            unset($data[$key]);
          }
        }
      }
      else if ($id == 'node_type_delete_confirm') {
        $data['op'] = t('Delete content type');
        $data['confirm'] = 1;
      }
      else if (in_array($id, array('_content_admin_field_add_existing', '_content_admin_field_add_new', '_content_admin_field'))) {
        $mappings = array('type' => 'type_name', 'widget' => 'widget_type', 'option' => 'field_type');
        
        foreach($mappings as $old => $new) {
          $data[$new] = $data[$old];
          unset($data[$old]);
        }
        
        if ($id == '_content_admin_field_add_new') {
          $data['field_widget_type'] = $data['field_type'] .'-'. $data['widget_type'];
        }
      }

      return $data;
    break;
    
    // Create extra parameters needed for a pattern execution
    case 'params':
      if ($id == 'node_type_delete_confirm') {
        // Make sure data is cached
        node_get_types('type', $type);
        
        $type = (object)content_types($data['type']);
        return $type;
      }
      else if ($id == '_content_admin_field_add_new' || $id == '_content_admin_field' || $id == '_content_admin_field_remove') {
        return array($data['type_name'], $data['field_name']);
      }
      else if ($id == '_content_admin_field_add_existing') {
        return $data['type_name'];
      }
      else if ($id == 'content_admin_display_overview_form') {
        return $data['type_name'];
      }
      else if ($id == 'node_type_form') {
        return node_get_types('type', $data['type']); 
      }
      else if ($id == 'fieldgroup_edit_group_form') {
        $group = db_result(db_query('SELECT group_name FROM {node_group} WHERE type_name = "%s" AND group_name = "%s"', $data['type'], $data['group_name']));
        $type = (array)node_get_types('type', $data['type']);  
        return array($type, $group, $group ? 'edit' : 'add');
      }
    break;
    
    // Handle disabling patterns
    case 'reverse':
      if ($id == 'content') {
        if (!$data['delete']) {
          $data['delete'] = true;
        }
        else {
          unset($data['delete']);
        }
      }
      else if ($id == 'field') {
        if (!$data['delete']) {
          $data['delete'] = true;
        }
        else {
          unset($data['delete']);
        }
      }
    break;
    
    // Any last cleanup items
    case 'cleanup':
      // Make sure content info is always up-to-date
      _content_type_info(true);
    break;
  }
}