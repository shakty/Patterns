<?php

function menu_patterns($op, $id = null, &$data = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('menu');
    break;
    
    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'menu_edit_item_form' => t('Edit a menu item'),
        'menu_edit_menu_form' => t('Edit a menu'),
        'menu_item_delete_form' => t('Delete a menu item'),
        'menu_reset_item' => t('Reset a menu item'),
        'menu_confirm_disable_item' => t('Disable a menu item'),
      );
    break;
    
    // Return a summary of an action
    case 'summary':
      switch($id) {
        case 'menu_edit_item_form':
          if (!db_result(db_query('SELECT mid FROM {menu} WHERE path = "%s"', $data['path']))) {
            return t('Create new menu item for %path', array('%path' => $data['path']));
          }
          else {
            return t('Edit menu item for %path', array('%path' => $data['path']));
          }
        break;
        case 'menu_edit_menu_form':
          if (!db_result(db_query('SELECT title FROM {menu} WHERE title = "%s"', $data['title']))) {
            return t('Create new menu %title', array('%title' => $data['title']));
          }
          else {
            return t('Edit menu %title', array('%title' => $data['title']));
          }
        break;
        case 'menu_item_delete_form':
        
        break;
        case 'menu_reset_item':
        
        break;
        case 'menu_confirm_disable_item':
        
        break;
      }
    break;
    
    // Prepare data for processing
    case 'prepare':
      // Automatically enabled when saved...
      unset($data['enable']);
      
      if (is_string($data['parent'])) {
        $data['parent'] = db_result(db_query('SELECT mid FROM {menu} WHERE title = "%s" ORDER BY pid ASC, type DESC', $data['parent']));
      }
      
      // Convert the easier tag names to their real names
      if ($data['id']) {
        $data['mid'] = $data['id'];
        unset($data['id']);
      }
      if ($data['parent']) {
        $data['pid'] = $data['parent'];
        unset($data['parent']);
      }
      
      // Use already existing menu items if they exist. Plan to make this configurable per menu item....
      if ($data['path'] && !$data['mid'] && ($mid = db_result(db_query('SELECT mid FROM {menu} WHERE path = "%s"', $data['path'])))) {
        $data['mid'] = $mid;
      }
      
      // New menu items can be only one of these two menu item types
      if (!$data['type'] && !$data['pid'] && !$data['mid']) {
        $data['type'] = MENU_CUSTOM_MENU;
        $data['pid'] = 0;
      }
      else if (!$data['type'] && !$data['mid']) {
        $data['type'] = MENU_CUSTOM_ITEM;
      }
      
      // Merge default values into the menu item
      if ($data['mid']) {
        $item = menu_get_item($data['mid']);
        if ($item) {
          unset($data['type']);
          $data = array_merge($item, $data);
        }
      }
      
      // Get mids from names for main menus
      if (!$data['mid'] && $data['title'] && !$data['path'] && ($mid = db_result(db_query('SELECT mid FROM {menu} WHERE title = "%s" AND pid = 0', $data['title'])))) {
        $data['mid'] = $mid;
      }
    break;
    
    // Pre validate actions
    case 'pre-validate':
      if (!$data['mid'] && $data['type'] && !in_array($data['type'], array(MENU_CUSTOM_MENU, MENU_CUSTOM_ITEM))) {
        return t('New menu item &lt;type&gt; tag can only contain one of these two values: !menu, !item', array('!menu' => MENU_CUSTOM_MENU, '!item' => MENU_CUSTOM_ITEM));
      }
    break;
    
    // Return the form_id('s) for each action
    case 'form_id':
      if ($data['delete']) {
        return 'menu_item_delete_form';
      }
      else if ($data['disable']) {
        return 'menu_confirm_disable_item';
      }
      else if ($data['reset']) {
        return 'menu_reset_item';
      }
      else if (!$data['path']) {
        return 'menu_edit_menu_form';
      }
      else {
        return 'menu_edit_item_form';
      }
    break;
    
    // Prepare for valid processing of this type of component
    case 'build':
      if ($id == 'menu_confirm_disable_item' || $id == 'menu_reset_item') {
        $data['confirm'] = 1;
      }
      
      return $data;
    break;
    
    // Validate the values for an action before running the pattern
    case 'validate':
      if ($data['mid'] && !($item = db_fetch_array(db_query('SELECT * FROM {menu} WHERE mid = %d', $data['mid'])))) {
        unset($data[$mid]);
        drupal_set_message(t('ID #!mid for menu item %title does not exist.', array('!mid' => $mid, '%title' => $data['title'])));
      }
    break;
    
    // Build a patterns actions and parameters
    case 'params':
      if ($id == 'menu_edit_item_form') {
        if (!$data['mid']) {
          return array('item');
        }
        else {
          return array('edit', $data['mid']);
        }
      }
      else if ($id == 'menu_edit_menu_form') {
        if (!$data['mid']) {
          return array('menu');
        }
        else {
          return array('edit', $data['mid']);
        }
      }
      else if ($data['mid']) {
        return $data['mid'];
      }
    break;
    
    // Reverse actions when disabling a pattern
    case 'reverse':
      if (!$data['delete'] && !$data['reset'] && !$data['disable']) {
        $data['delete'] = true;
      }
      else if ($data['disable']) {
        unset($data['disable']);
      }
      else if ($data['reset']) {
        drupal_set_message(t('Unable to reverse <menu> tags that have %reset set.', array('%reset' => t('reset'))));
      }
      else if ($data['enable']) {
        $data['disable'] = true;
        unset($data['enable']);
      }
      else if ($data['delete']) {
        unset($data['delete']);
      }
      return $data;
    break;
    
    // Cleanup any global settings after the action runs
    case 'cleanup':
      unset($_POST['op']);
      
      // Delete menu cache
      menu_rebuild();
    break;
    
    // Return the primary ID if possible from this action
    case 'identifier':
      if ($data['mid']) {
        return $data['mid'];
      }
      else if ($data['path']) {
        return db_result(db_query('SELECT mid FROM {menu} WHERE path = "%s"', $data['path']));
      }
      else if ($data['title']) {
        return db_result(db_query('SELECT mid FROM {menu} WHERE title = "%s" ORDER BY pid ASC, type DESC', $data['title']));
      }
    break;
  }
}