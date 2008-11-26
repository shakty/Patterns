<?php

function menu_patterns($op, $id = null, &$data = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('menu', 'menu_item');
    break;
    
    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'menu_edit_menu' => t('Add/edit menu'),
        'menu_delete_menu_confirm' => t('Delete menu'), 
        'menu_edit_item' => t('Add/edit menu item'),
        'menu_item_delete_form' => t('Delete a menu item'),
        'menu_overview_form' => t('Configure menu item'),
      );
    break;
    
    // Return a summary of an action
    case 'summary':
      switch($id) {
        case 'menu_edit_item':

        break;
        case 'menu_edit_menu':

        break;
        case 'menu_item_delete_form':
        
        break;
        case 'menu_overview_form':
        
        break;
        case 'menu_delete_menu_confirm':
        
        break;
      }
    break;
    
    // Prepare data for processing
    case 'prepare':
//       // Automatically enabled when saved...
//       unset($data['enable']);
      switch ($id) {
        case 'menu':
          if (empty($data['menu_name']) && !empty($data['title']) && $menu_name = db_result(db_query("SELECT menu_name FROM {menu_custom} WHERE title = '%s'", $data['title']))) {
            $data['menu_name'] = $menu_name;
          }
          if (empty($data['menu_name']) && !empty($data['title'])) {
            $data['menu_name'] = strtolower(preg_replace('/[^a-zA-Z0-9\-]/', '-', $data['title']));
          }
        break;
        case 'menu_item':
          
          dpm($data);
        
          if (empty($data['menu_name']) && !empty($data['menu']) && $menu_name = db_result(db_query("SELECT menu_name FROM {menu_custom} WHERE title = '%s'", $data['menu']))) {
            $data['menu_name'] = $menu_name;
            unset($data['menu']);
          }
          
          if (is_numeric($data['id'])) {
            $data['mlid'] = $data['id'];
            unset($data['id']);
          }
          
          if (is_string($data['parent']) && !empty($data['menu_name']) && $plid = db_result(db_query("SELECT plid FROM {menu_links} WHERE menu_name = '%s' AND link_title = '%s'", $data['menu_name'], $data['parent']))) {
            $data['parent'] = $data['menu_name'] .':'. $plid; 
          }
          else if (is_numeric($data['parent']) && !empty($data['menu_name']) && db_result(db_query("SELECT mlid FROM {menu_links} WHERE menu_name = '%s' AND mlid = %d", $data['menu_name'], $data['parent']))) {
            $data['parent'] = $data['menu_name'] .':'. $data['parent'];
          }
          else if (empty($data['mlid']) && empty($data['parent']) && !empty($data['title']) && !empty($data['path'])) {
            $data['parent'] = $data['menu_name'] .':0';
          }
          
          dpm($data);
          
          if (($data['delete'] || $data['disable'] || $data['enable']) && empty($data['mlid'])) {
            $data['mlid'] = db_result(db_query("SELECT mlid FROM {menu_links} WHERE menu_name = '%s' AND link_path = '%s' AND link_title = '%s'", $data['menu_name'], $data['path'], $data['title']));
          }

          if (!empty($data['path'])) $data['link_path'] = $data['path'];
          unset($data['path']);
          if (!empty($data['title'])) $data['link_title'] = $data['title'];
          unset($data['title']);
          
          dpm($data);
        break;
      }
    break;
    
    // Pre validate actions
    case 'pre-validate':
      // @todo all errors should be collected and displayed at once to the user (instead of exiting at first encountered error)
      switch ($id) {
        case 'menu':
          if ($data['delete'] && empty($data['menu_name'])) {
            return t('"menu_name" is required.');
          }
        break;
        case 'menu_item':
          if (empty($data['mlid']) && empty($data['menu_name'])) {
            return t('"menu_name" is required.');
          }
          if (!empty($data['parent'])) {
            list($menu_name, $mlid) = explode(':', $data['parent']);
            if ($mlid != 0 && !db_result(db_query("SELECT count(*) FROM {menu_links} WHERE menu_name = '%s' AND mlid = %d", $menu_name, $mlid))) {
              return t('parent is not valid.');
            }
          }
          else if (empty($data['mlid'])) {
            return t('"parent" is required.');
          }
          if (empty($data['mlid'])) {
            if(empty($data['link_path'])) {
              return t('"path" is required.');
            }
            if(empty($data['link_title'])) {
              return t('"title" is required.');
            }
//             if ($data['delete']) {
//               return t('You have to supply more information in order to determine which menu itme needs to be deleted. Either provide "mlid" or all three following fields: "menu_name", "path", "title"');
//             }                   
          }
        break;
      }  
    break;
    
    // Return the form_id('s) for each action
    case 'form_id':
      if ($id == 'menu') {
        if ($data['delete']) {
          return 'menu_delete_menu_confirm';
        }
        else {
          return 'menu_edit_menu';
        }              
      }
      else if ($id == 'menu_item') {
        if ($data['delete']) {
          return 'menu_item_delete_form';
        }
        else if ($data['disable'] || $data['enable']) {
          return 'menu_overview_form';
        }
//         else if ($data['reset']) {
//           return 'menu_reset_item_confirm';
//         }        
        else {
          return 'menu_edit_item';
        }        
      }
    break;
    
    // Prepare for valid processing of this type of component
    case 'build':
      module_load_include('inc', 'menu', 'menu.admin');
      if ($id == 'menu_item_delete_form' || $id == 'menu_delete_menu_confirm') {
        $data['confirm'] = 1;        
      }
      else if ($id == 'menu_overview_form') {
        if ($data['disable']) {
          $data['hidden'] = 0;
          unset($data['disable']);          
        }
        else if ($data['enable']) {
          $data['hidden'] = 1;
          unset($data['enable']);        
        }
//         list( ,$data['plid']) = explode(':', $data['parent']); 
//         unset($data['link_path'], $data['link_title'], $data['parent']);
        $d = array();
        $d['mlid:'. $data['mlid']] = $data;
        $data = $d;        
      }
      else if ($id == 'menu_reset_item_confirm') {
      
      }      
      else if ($id == 'menu_edit_item') {
        $data['customized'] = 1;
        $data['module'] = 'menu';
        if ($data['disable']) {
          $data['enabled'] = 0;
          unset($data['disable']);
        }
        $d = array();
        $d['menu'] = $data;
        $data = $d; 
      }
      dpm($data);
      return $data;
    break;
    
    // Validate the values for an action before running the pattern
    case 'validate':
//       if ($data['mid'] && !($item = db_fetch_array(db_query('SELECT * FROM {menu} WHERE mid = %d', $data['mid'])))) {
//         unset($data[$mid]);
//         drupal_set_message(t('ID #!mid for menu item %title does not exist.', array('!mid' => $mid, '%title' => $data['title'])));
//       }
    break;
    
    // Build a patterns actions and parameters
    case 'params':
      switch ($id) {
        case 'menu_edit_menu':
          $menu = menu_load($data['menu_name']);
          if (empty($menu)) {
            return 'add';
          }
          else {
            return array('edit', $menu);
          }
        break;
        case 'menu_delete_menu_confirm':
          return array(menu_load($data['menu_name']));
        break;
        case 'menu_edit_item':
          dpm($data);
          if (!$data['menu']['mlid']) {
            return array('add', NULL, menu_load($data['menu']['menu_name']));
          }
          else {
            return array('edit', menu_link_load($data['menu']['mlid']), NULL);
          }        
        break;
        case 'menu_item_delete_form':
          return array(menu_link_load($data['mlid']));
        break;
        case 'menu_overview_form':
          $item = reset($data);
          dpm($item);
          dpm(menu_load($item['menu_name']));
          return array(menu_load($item['menu_name']));
        break;
      }
    break;
    
    // Cleanup any global settings after the action runs
    case 'cleanup':
      unset($_POST['op']);
      
      // Delete menu cache
      menu_rebuild();
    break;
    
    // Return the primary ID if possible from this action
    case 'identifier':
      if ($id == 'menu_edit_menu') {
        return $data['menu_name'];        
      }
      else if ($id == 'menu_edit_item' || $id == 'menu_overview_form') {
        $data = reset($data);
        if ($data['mlid']) {
          return $data['mlid'];
        }
        else if ($data['link_path']) {
          return db_result(db_query('SELECT mlid FROM {menu_links} WHERE link_path = "%s" AND menu_name = "%s"', $data['link_path'], $data['menu_name']));
        }
        else if ($data['link_title']) {
          return db_result(db_query('SELECT mlid FROM {menu_links} WHERE link_title = "%s" AND menu_name = "%s"', $data['link_title'], $data['menu_name']));
        }              
      }
    break;
  }
}
