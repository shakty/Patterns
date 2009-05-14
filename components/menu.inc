<?php

function menu_patterns($op, $id = null, &$data = null, &$a = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('menu', 'menu_item');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'menu_edit_menu' => t('Menu: Add or edit menu'),
        'menu_delete_menu_confirm' => t('Menu: Delete menu'),
        'menu_edit_item' => t('Menu: Add or edit menu item'),
        'menu_item_delete_form' => t('Menu: Delete menu item'),
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
        case 'menu_delete_menu_confirm':

        break;
      }
    break;

    // Prepare data for processing
    case 'prepare':
      switch ($id) {
        case 'menu':
          if (empty($data['menu_name']) && !empty($data['name'])) {
            $data['menu_name'] = $data['name'];
            unset($data['name']);
          }

          if (empty($data['menu_name']) && !empty($data['title'])) {
            $data['menu_name'] = strtolower(preg_replace('/[^a-zA-Z0-9\-]/', '-', $data['title']));
            if (!menu_load($data['menu_name'])) {
              $data['menu_name'] = 'menu-'. $data['menu_name'];
            }
          }
        break;
        case 'menu_item':
          if (!empty($data['id'])) {
            $data['mlid'] = $data['id'];
            unset($data['id']);
          }

          if (!empty($data['path'])) $data['link_path'] = $data['path'];
          unset($data['path']);
          if (!empty($data['title'])) $data['link_title'] = $data['title'];
          unset($data['title']);

          if (!empty($data['link_path'])) $data['link_path'] = str_replace(array('[', ']'), array('<', '>'), $data['link_path']);

        break;
      }
    break;

    // Pre validate actions
    case 'pre-validate':
      switch ($id) {
        case 'menu':
          if ($data['delete'] && empty($data['menu_name'])) {
            return t('"menu_name" is required.');
          }
          if (!empty($data['menu_name']) && preg_match('/[^a-z0-9\-]/', $data['menu_name'])) {
            return t('The menu name may only consist of lowercase letters, numbers, and hyphens: %menu', array('%menu' => $data['menu_name']));
          }
        break;
        case 'menu_item':
          if (empty($data['mlid'])) {
            if(empty($data['link_path'])) {
              return t('"path" is required.');
            }
            if(empty($data['link_title'])) {
              return t('"title" is required.');
            }
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
      else if ($id == 'menu_reset_item_confirm') {
        // @TODO
      }
      else if ($id == 'menu_edit_item') {
        $data['customized'] = 1;
        $data['module'] = 'menu';
        if ($data['disable']) {
          $data['enabled'] = 0;
          unset($data['disable']);
        }
        elseif ($data['enable']) {
          $data['enabled'] = 1;
          unset($data['enable']);
        }
        $d = array();
        $d['menu'] = $data;
        $data = $d;
      }
      return $data;
    break;

    // Validate the values for an action before running the pattern
    case 'validate':
      switch ($id) {
        case 'menu_edit_item':

          if (empty($data['menu_name']) && !empty($data['menu'])) {
            if ($count = db_result(db_query("SELECT COUNT(*) FROM {menu_custom} WHERE title = '%s'", $data['menu']))) {
              if ($count == 1) {
                $data['menu_name'] = db_result(db_query("SELECT menu_name FROM {menu_custom} WHERE title = '%s'", $data['menu']));
                unset($data['menu']);
              }
              else {
                return t("Menu name couldn't be determined accurately because more then one menu has exactly the same title. You need to provide menu_name instead of the title: '%title'", array('%title' => $data['menu']));
              }
            }
          }

          if ((empty($data['mlid']) || isset($data['parent'])) && empty($data['menu_name'])) {
            return t('"menu_name" is required.');
          }

          if ($data['parent'] == '0') {
            $data['parent'] = $data['menu_name'] .':0';
          }
          else if (is_numeric($data['parent']) && !empty($data['menu_name']) && db_result(db_query("SELECT mlid FROM {menu_links} WHERE menu_name = '%s' AND mlid = %d", $data['menu_name'], $data['parent']))) {
            $data['parent'] = $data['menu_name'] .':'. $data['parent'];
          }
          // @TODO this may not work if there is more then one menu item with the same title within the same menu
          else if (is_string($data['parent']) && !empty($data['menu_name']) && $plid = db_result(db_query("SELECT mlid FROM {menu_links} WHERE menu_name = '%s' AND link_title = '%s'", $data['menu_name'], $data['parent']))) {
            $data['parent'] = $data['menu_name'] .':'. $plid;
          }
          else if (empty($data['mlid']) && empty($data['parent']) && !empty($data['link_title']) && !empty($data['link_path'])) {
            $data['parent'] = $data['menu_name'] .':0';
          }

          if (!empty($data['parent'])) {
            list($menu_name, $mlid) = explode(':', $data['parent']);
            if ($mlid != 0 && !db_result(db_query("SELECT count(*) FROM {menu_links} WHERE menu_name = '%s' AND mlid = %d", $menu_name, $mlid))) {
              return t('"parent" is not valid.');
            }
          }
          else if (empty($data['mlid'])) {
            return t('"parent" is required.');
          }
        break;
      }
    break;

    // Build a patterns actions and parameters
    case 'params':
      switch ($id) {
        case 'menu_edit_menu':
          $menu = menu_load($data['menu_name']);
          if (empty($menu)) {
            if (strpos($data['menu_name'], 'menu-') === 0) {
              // this ensures that same pattern can be re-run without getting "menu exists" error
              // $a is $form_state array that will be passed to drupal_execute()
              $a['values']['menu_name'] = str_replace('menu-', '', $data['menu_name']);
            }
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
      else if ($id == 'menu_edit_item') {
        $data = reset($data);
        if ($data['mlid']) {
          return $data['mlid'];
        }
        else if ($mlid = db_result(db_query("SELECT mlid FROM {menu_links} WHERE menu_name = '%s' AND link_path = '%s' AND link_title = '%s' ORDER BY mlid DESC", $data['menu_name'], $data['link_path'], $data['link_title']))) {
          return $mlid;
        }
      }
    break;
  }
}
