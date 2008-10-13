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
        'views_edit_view' => t('Create/Edit View'),
        'views_ui_admin_delete_confirm' => t('Delete View'),
      );
    break;
    
    // Return a summary of an action
    case 'summary':
      $name = strtolower($data['name']);
      switch($id) {
        case 'views_edit_view':
          if ($view = views_get_view($name)) {
            return t('Update view %view', array('%view' => $view->description));
          }
          else {
            foreach(array('description', 'page_title', 'block_title', 'menu_title', 'name') as $key) {
              if ($data[$key]) {
                $description = $data[$key];
                break;
              }
            }
            
            return t('Create view %view', array('%view' => $description));
          }
        break;
        case 'views_ui_admin_delete_confirm':
          if (!$data['name'] && $data['vid']) {
            $data['name'] = db_query('SELECT name FROM {view_view} WHERE vid = "%d"', $data['vid']);
          }
          
          return t('Delete view %view', array('%view' => $data['name']));
        break;
      }
    break;
    
    // Prepare data for views-centric structure
    case 'prepare':
      $data['name'] = strtolower($data['name']);

      // Map some base tags
      $mappings = views_patterns_mappings('base');

      foreach($data as $key => $value) {
        if ($mappings[$key]) {
          $data[$mappings[$key]] = $value;
          unset($data[$key]);
        }
      }
      
      // Make sure data structure is setup how we want it
      foreach(array('page', 'block', 'menu') as $type) {
        if ($data[$type] && is_array($data[$type])) {
          $mappings = views_patterns_mappings($type);
          foreach($data[$type] as $key => $value) {
            if ($mappings[$key]) {
              $data[$mappings[$key]] = $value;
              unset($data[$key]);
            }
          }
          $data[$type] = 1;
        }
      }

      if ($data['exposed']) {
        $data['exposed_filter'] = $data['exposed'];
        unset($data['exposed']);
      }
        
      foreach(array('field', 'exposed_filter', 'filter', 'sort', 'argument') as $type) {
        if (!is_array($data[$type])) {
          unset($data[$type]);
          continue;
        }
        else if (!is_numeric(key($data[$type]))) {
          $data[$type] = array($data[$type]);
        }
        
        foreach($data[$type] as $index => $item) {
          foreach($item as $key => $value) {
            if (in_array($key, array('operator', 'sortorder')) && is_string($value)) {
              $data[$type][$index][$key] = strtoupper($value);
            }
            else if ($key == 'type' && !$data[$type][$index]['id']) {
              $data[$type][$index]['id'] = $data[$type][$index][$key];
              unset($data[$type][$index][$key]);
            }
            else if ($key == 'value' && $type == 'sort') {
              $data[$type][$index]['sortorder'] = $value;
              unset($data[$type][$index][$key]);
              $key = 'sortorder';
              
            }
            
            if ($key == 'sortorder') {
              $sorts = _views_sortorders();
              array_walk($sorts, create_function('&$a', '$a =strtolower($a);'));

              if ($i = array_search(strtolower($value), $sorts)) {
                $data[$type][$index][$key] = $i;
              }
            }
          }
        }
      }
      
      foreach($data['filter'] as $index => $filter) {
        if ($filter['value'] && is_array($filter['value'])) {
          foreach($filter['value'] as $key => $value) {
            if ($key != $value) {
              $data['filter'][$index]['value'][$value] = $value;
              unset($data['filter'][$index]['value'][$key]);
            }
          }
        }
      }
      
      if (!$data['vid'] && $data['name']) {
        $data['vid'] = db_result(db_query('SELECT vid FROM {view_view} WHERE name = "%s"', $data['name']));
      }
    break;
    
    // Validate and cleanup the values for an action before running the pattern
    case 'pre-validate':
      $name = $data['name'];
      
      if (!$name) {
        return t('Missing tag &lt;type&gt; inside of &lt;content&gt;');
      }
      
      if (preg_match('/ |-/', $data['name'])) {
        return t('Only characters, numbers, and underscores are allowed in the views &lt;name&gt; tag.');
      }
    break;
    
    // Ask the user if its ok to do certain tasks
    case 'confirmation form':
    break;
    
    // Validate action before processing
    case 'validate':
      views_load_cache();
      views_invalidate_cache();
      
      if ($id == 'views_edit_view') {
        $tables = _views_get_tables();
        
        $errors = array();
        foreach(array('field', 'filter', 'argument', 'sort') as $type) {
          foreach((array)$data[$type] as $key => $value) {
            list($tablename, $fieldname) = explode('.', $value['id']);
            
            if (!$tables[$tablename]) {
              $errors[] = t('Invalid table @table in @id', array('@table' => $tablename, '@id' => $value['id']));
            }
            else if (!$tables[$tablename]['fields'][$fieldname]) {
              $errors[] = t('Invalid field @field in @id', array('@field' => $fieldname, '@id' => $value['id'])); 
            }
          }
        }
      }
      else if ($id == 'views_ui_admin_delete_confirm') {
        if (!($view = _views_load_view($data['vid']))) {
          return t('Error deleting view.');
        }
      }
    break;
    
    // Return the form id or determine if the action does not need processing
    case 'form_id':
      if ($data['delete']) {
        if (!is_numeric($data['vid'])) {
          return;
        }
        
        return 'views_ui_admin_delete_confirm';
      }
      else if ($data['value']) {
        return 'views_ui_admin_import';
      }
      else {
        return 'views_edit_view';
      }
    break;
    
    // Add default values to the pattern where appropriate and return form data 
    case 'build':
      if ($id == 'views_edit_view') {
        $tables = _views_get_tables();
        
        // Make sure [order] and [count] values are set
        $check_order = create_function('$a, $b, $c=false', 'static $order = array(); if (is_numeric($b)) $order[] = $b; if ($c) { $temp = $order; $order = array(); return $temp; }');
        foreach(array('filter', 'sort', 'field', 'argument', 'exposed_filter') as $type) {
          if (is_array($data[$type])) {
            array_walk($data[$type], $check_order);
            //$data[$type]['order'] = $check_order(null, null, true);
            //$data[$type]['count'] = count($data[$type]['order']);
          }
        }
        
        // Make sure necessary field values are set
        foreach((array)$data['field'] as $index => $field) {
          if (!is_numeric($index)) {
            continue;
          }
          
          if (!$field['fullname']) {
            $field['fullname'] = $field['id'];
          }
          
          if (!$field['queryname']) {
            $field['queryname'] = str_replace('.', '_', $field['id']);
          }
          
          if (!$field['tablename']) {
            $split = explode('.', $field['id']);
            $field['tablename'] = $split[0];
          }
          
          if (!$field['field']) {
            $split = explode('.', $field['id']);
            $field['field'] = $split[1];
          }
          
          if (!isset($field['sortable'])) {
            $field['sortable'] = 0;
          }
          
          if (!isset($field['defaultsort'])) {
            $field['defaultsort'] = 0;
          }
          
          if ($f = $tables[$field['tablename']]['fields'][$field['field']]) {
            if (!$field['handler'] && $f['handler']) {
              if (is_array($f['handler'])) {
                $field['handler'] = key($f['handler']);
              }
              else {
                $field['handler'] = $f['handler'];
              }
            }
            
            if (!$field['option'] && $f['option']) {
              $field['option'] = 0;
            }
          }
          
          $data['field'][$index] = $field;
        }
        
        // Make sure necessary argument values are set
        foreach((array)$data['argument'] as $index => $arg) {
          if (!is_numeric($index)) {
            continue;
          }
          
          if (!$arg['type']) {
            $arg['type'] = $arg['id'];
          }
          
          if (!$arg['argdefault']) {
            if ($arg['default']) {
              $arg['argdefault'] = $arg['default'];
            }
            else {
              $arg['argdefault'] = 1;
            }
          }
          
          $data['argument'][$index] = $arg;
        }
        
        // Make sure necessary filter values are set
        foreach((array)$data['filter'] as $index => $filter) {
          if (!is_numeric($index)) {
            continue;
          }
          
          if (!$filter['field']) {
            $filter['field'] = $filter['id'];
          }
          
          list($tablename, $fieldname) = explode('.', $filter['id']);
          
          if ($f = $tables[$field['tablename']]['filters'][$filter['field']]) {
            if (!$filter['operator'] && ($operator = $f['operator'])) {
              if (!is_array($operator) && function_exists($operator))  {
                $operator = $operator('operator', $f);
              }
              $operator = key($operator);
              
              $filter['operator'] = key($operator);
            }
            
            if (!$filter['value'] && ($value = views_ui_setup_widget($f['value'], '', $f)) && isset($value['#options'])) {
              $filter['value'] = key($value['#options']);
            }
          }
          
          $data['filter'][$index] = $filter;
        }
        
        // Make sure necessary sort values are set
        foreach((array)$data['sort'] as $index => $sort) {
          if (!is_numeric($index)) {
            continue;
          }
          
          if (!$sort['field']) {
            $sort['field'] = $sort['id'];
          }
          
          if (!$sort['sortorder']) {
            $sorts = _views_sortorders();
            $sort['sortorder'] = key($sorts);
          }
          
          $data['sort'][$index] = $sort;
        }
        
        // Make sure necessary exposed values are set
        foreach((array)$data['exposed_filter'] as $index => $exposed) {
          if (!is_numeric($index)) {
            continue;
          }
          
          if (!$exposed['field']) {
            if (!$exposed['field']) {
              $exposed['field'] = $exposed['id'];
            }
          }
          
          $data['exposed_filter'][$index] = $exposed;
        }
      }
      else if ($id == 'views_ui_admin_delete_confirm') {
        _views_delete_view((object) $data);
        menu_rebuild();
        
        return t('View %name removed.', array('%name' => $data['name']));
      }
      
      return $data;
    break;
    
    // Create extra parameters needed for a pattern execution
    case 'params':
      if ($id == 'views_edit_view') {
        return array((object)$data, '');
      }
      else if ($id == 'views_ui_admin_delete_confirm') {
        return $data['vid'];
      }
    break;
    
    // Handle disabling patterns
    case 'reverse':
      if (!$data['delete']) {
        $data['delete'] = true;
      }
      else {
        unset($data['delete']);
      }
    break;
    
    // Return the default values for an action
    case 'defaults':
      if ($id == 'views_edit_view') {
        return array(
          'vid' => 0,
          'allbut' => '',
          'changed' => '',
          'name' => '',
          'access' => array(),
          'description' => '',
          'page' => 0,
          'url' => '',
          'page_type' => 'node',
          'page_title' => '',
          'use_pager' => 1,
          'breadcrumb_no_home' => 0,
          'nodes_per_page' => 10,
          'page_header' => '',
          'page_header_format' => 3,
          'page_footer' => '',
          'page_footer_format' => 2,
          'page_empty' => '',
          'page_empty_format' => 1,
          'menu' => 1,
          'menu_tab' => 1,
          'menu_tab_weight' => -9,
          'menu_title' => '',
          'menu_tab_default' => 0,
          'menu_tab_default_parent_type' => 'tab',
          'menu_parent_tab_weight' => '',
          'menu_parent_title' => '',
          'block' => 0,
          'block_type' => 'node',
          'block_title' => '',
          'nodes_per_block' => '',
          'block_more' => 0,
          'block_use_page_header' => 0,
          'block_header' => '',
          'block_header_format' => 1,
          'block_use_page_footer' => 0,
          'block_footer' => '',
          'block_footer_format' => 1,
          'block_use_page_empty' => 0,
          'block_empty' => '',
          'block_empty_format' => 1,
          'view_args_php' => '',
        );
      }
    break;
  }
}

function views_patterns_mappings($type) {
  switch($type) {
    case 'page':
      $mappings = array('type' => 'page_type', 'title' => 'page_title', 'pager' => 'use_pager', 'limit' => 'nodes_per_page', 'numnodes' => 'nodes_per_page', 'count' => 'nodes_per_page', 'header' => 'page_header', 'header_format' => 'page_header_format', 'footer' => 'page_footer', 'footer_format' => 'page_footer_format', 'empty' => 'page_empty', 'empty_format' => 'page_empty_format');
    break;
    case 'block':
      $mappings = array('type' => 'block_type', 'title' => 'block_title', 'limit' => 'nodes_per_block', 'numnodes' => 'nodes_per_block', 'more' => 'block_more', 'header' => 'block_header', 'header_format' => 'block_header_format', 'page_header' => 'block_use_page_header', 'footer' => 'block_footer', 'footer_format' => 'block_footer_format', 'page_footer' => 'block_use_page_footer', 'empty' => 'block_empty', 'empty_format' => 'block_empty_format', 'page_empty' => 'block_use_page_empty');
    break;
    case 'menu':
      $mappings = array('tab' => 'menu_tab', 'tab_weight' => 'menu_tab_weight', 'title' => 'menu_title', 'tab_default' => 'menu_tab_default', 'tab_default_parent_type' => 'menu_tab_default_parent_type', 'parent_tab_weight' => 'menu_parent_tab_weight', 'parent_title' => 'menu_parent_title');
    break;
    case 'base':
      $mappings = array('argphp' => 'view_args_php', 'php' => 'view_args_php');
    break;
  }
  
  return $mappings;
}
