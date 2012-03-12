<?php
/*
 * @file
 * Patterns component for the menu system.
 */

function menu_patterns($data) {
  $files = array('modules/menu/menu.admin.inc');

  $actions['menu'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete menu'),
    PATTERNS_CREATE => array('menu_edit_menu'),
    PATTERNS_MODIFY => array('menu_edit_menu'),
    PATTERNS_DELETE => array('menu_delete_menu_confirm'),
    PATTERNS_FILES => $files,
  );
  $actions['menu_item'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete menu item'),
    PATTERNS_CREATE => array('menu_edit_item'),
    PATTERNS_MODIFY => array('menu_edit_item'),
    PATTERNS_DELETE => array('menu_item_delete_form'),
    PATTERNS_FILES => $files,
  );

  return $actions;

}

function menu_patterns_actions($data) {
  return array(
    'menu_edit_menu' => t('Menu: Add or edit menu'),
    'menu_delete_menu_confirm' => t('Menu: Delete menu'),
    'menu_edit_item' => t('Menu: Add or edit menu item'),
    'menu_item_delete_form' => t('Menu: Delete menu item'),
  );
}

// Prepare data for processing
function menu_patterns_prepare($action, $tag, &$data) {
  if ($action == PATTERNS_DELETE) {
    $data['confirm'] = 1;
  }
  switch ($tag) {
    case 'menu':
      if (empty($data['menu_name']) && !empty($data['name'])) {
        $data['menu_name'] = $data['name'];
        unset($data['name']);
      }
      break;
    case 'menu_item':
      if (!empty($data['id'])) {
        $data['mlid'] = $data['id'];
        unset($data['id']);
      }

      if (!empty($data['path'])) {
        $data['link_path'] = $data['path'];
      }
      unset($data['path']);
      if (!empty($data['title'])) {
        $data['link_title'] = $data['title'];
      }
      unset($data['title']);

      if (!empty($data['link_path'])) {
        $data['link_path'] = str_replace(array('[', ']'), array('<', '>'), $data['link_path']);
      }

      if (!empty($data['menu_name'])) {
        if (!isset($data['parent'] ) || $data['parent'] == '0') {
          $data['parent'] = $data['menu_name'] . ':0';
        }
        elseif (is_numeric($data['parent'])    && db_query("SELECT mlid FROM {menu_links} WHERE menu_name = :menu AND mlid = :mlid", array('menu' => $data['menu_name'], 'mlid' => $data['parent']))->fetchField()) {
          $data['parent'] = $data['menu_name'] . ':' . $data['parent'];
        }
        // @TODO this may not work if there is more than one menu item with the same title within the same menu
        elseif (is_string($data['parent']) && $plid = db_query("SELECT mlid FROM {menu_links} WHERE menu_name = :menu AND link_title = :title", array('menu' => $data['menu_name'], 'title' => $data['parent']))->fetchField()) {
          $data['parent'] = $data['menu_name'] . ':' . $plid;
        }
        elseif (empty($data['mlid']) && empty($data['parent']) && !empty($data['link_title']) && !empty($data['link_path'])) {
          $data['parent'] = $data['menu_name'] . ':0';
        }
      }

      if ($action == PATTERNS_MODIFY) {
        $data['customized'] = 1;
        $data['module'] = 'menu';
        if ($data['disable']) {
          $data['enabled'] = 0;
          unset($data['disable']);
        }
        elseif ($data['enable']) {
          $data['enabled'] = 1;
          unset($data['disable']);
        }
      }

      break;
  }
  return patterns_results();
}

/*
 * 
 */
function menu_patterns_get_arguments($action, $tag, $form_id, &$loop = FALSE) {
  // taken from menu_load_links
  if ($tag == 'menu_item' && $action == PATTERNS_MODIFY && $form_id == 'menu_edit_item') {
    $links = db_select('menu_links', 'ml', array('fetch' => PDO::FETCH_ASSOC))
      ->fields('ml')
      ->execute()
      ->fetchAll();
  //print_r($links); //for debug
  $loop = TRUE;
  $return = array();
  $i = 0;
  foreach ($links as &$link) {
    $i++;
    if ($link['link_title'] == '') {
      $link['link_title'] = 'null';
    }
    $link['options'] = unserialize($link['options']);
    $link['href'] = "";
    array_push($return, array("edit", $link, $link));
  }
  return $return;
}

// taken from menu_overview_page
  elseif ($tag == 'menu' && $action == PATTERNS_MODIFY && $form_id == 'menu_edit_menu') {
    $loop = TRUE;
    $result = db_query("SELECT * FROM {menu_custom} ORDER BY title", array(), array('fetch' => PDO::FETCH_ASSOC));
    $return = array();
    foreach ($result as $menu) {
      array_push($return, array("edit", $menu));
    }
    return $return;
  }

}


// Validate the values for an action before running the pattern
function menu_patterns_validate($action, $tag, &$data) {
  $status = PATTERNS_SUCCESS;
  $msg = '';

  if ($tag == "menu") {
    if (empty($data['menu_name'])) {
      $msg = t('The menu name is required.');
    }
    elseif (!empty($data['menu_name']) && preg_match('/[^a-z0-9\-]/', $data['menu_name'])) {
      $status = PATTERNS_ERR;
      $msg = t('The menu name may only consist of lowercase letters, numbers, and hyphens: %menu', array('%menu' => $data['menu_name']));
    }
    else {
      $count = db_query("SELECT COUNT(*) FROM {menu_custom} WHERE menu_name = :menu", array('menu' => $data['menu_name']))->fetchField();
      if ($action == PATTERNS_DELETE && !$count) {
        $status = PATTERNS_ERR;
        $msg = t('Attempt to delete a nonexistent menu');
      }
      elseif ($action == PATTERNS_MODIFY && !$count) {
        $status = PATTERNS_ERR;
        $msg = t('Attempt to update a nonexistent menu');
      }
      elseif ($action == PATTERNS_CREATE ) {
        $count = db_query("SELECT COUNT(*) FROM {menu_custom} WHERE menu_name = :menu", array('menu' => 'menu-' . $data['menu_name']))->fetchField();
        if ($count) {
          $status = PATTERNS_ERR;
          $msg = t('The machine-readable menu name is already in use. It must be unique.');
        }
      }
    }

  }
  elseif ($tag == "menu_item") {
    if ($action == PATTERNS_MODIFY && !isset($data['mlid'])) {
      if (empty($data['menu_name_old'])) {
        $status = PATTERNS_ERR;
        $msg =  t('"menu_name_old" or "mlid" is required.');
      }
      elseif (empty($data['link_path_old'])) {
        $status = PATTERNS_ERR;
        $msg =  t('"link_path_old" or "mlid" is required.');
      }
      elseif (empty($data['link_title_old'])) {
        $status = PATTERNS_ERR;
        $msg =  t('"link_title_old" or "mlid" is required.');
      }
    }
    if ($action !== PATTERNS_DELETE || ($action == PATTERNS_DELETE && !isset($data['mlid']))) {
      if (empty($data['menu_name'])) {
        $status = PATTERNS_ERR;
        $msg =  t('"menu_name" is required.');
      }
      elseif (empty($data['link_path'])) {
        $status = PATTERNS_ERR;
        $msg =  t('"path" is required.');
      }
      elseif (empty($data['link_title'])) {
        $status = PATTERNS_ERR;
        $msg =  t('"title" is required.');
      }
    }
    if (!empty($data['parent']) && $action !== PATTERNS_DELETE) {
      list($menu_name, $mlid) = explode(':', $data['parent']);
      if ($mlid != 0 && !db_query("SELECT count(*) FROM {menu_links} WHERE menu_name = :menu AND mlid = :mlid", array('menu' => $menu_name, 'mlid' => $mlid))->fetchField()) {
        $status = PATTERNS_ERR;
        $msg = t('"parent" is not valid.');
      }
    }
  }
  return patterns_results($status, $msg);
}

// Prepare for valid processing of this type of component
function menu_patterns_build($action, $tag, &$data, $form_id) {
  module_load_include('inc', 'menu', 'menu.admin');
  return patterns_results(PATTERNS_SUCCESS,    t('Execution successful'), $data);
}

// Build a patterns actions and parameters
function menu_patterns_params($action, $form_id, &$data, &$a) {
  switch ($form_id) {
    case 'menu_edit_menu':

      if ($action == PATTERNS_CREATE) {
        $result = 'add';
      }
      elseif ($action == PATTERNS_MODIFY ) {
        $menu = menu_load($data['menu_name']);
        $result = array('edit', $menu);
      }
      break;
    case 'menu_delete_menu_confirm':
      $result = array(menu_load($data['menu_name']));
      break;
    case 'menu_edit_item':
      if ($action == PATTERNS_CREATE) {
        $result = array('add', NULL, menu_load($data['menu_name']));
      }
      elseif ($action == PATTERNS_MODIFY) {
        if (!isset($data['mlid'])) {
          $mlid = db_query("SELECT mlid FROM {menu_links} WHERE menu_name = :menu and link_path=:path and link_title=:title", array('menu' => $data['menu_name_old'], 'path' => $data['link_path_old'], 'title' => $data['link_title_old']))->fetchField();
        }
        $result = array('edit', menu_link_load($mlid), NULL);
      }
      break;
    case 'menu_item_delete_form':
      if (!isset($data['mlid'])) {
        $mlid = db_query("SELECT mlid FROM {menu_links} WHERE menu_name = :menu and link_path=:path and link_title=:title", array('menu' => $data['menu_name'], 'path' => $data['link_path'], 'title' => $data['link_title']))->fetchField();
      }
      $result = array(menu_link_load($mlid));
      break;
  }
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

function menu_patterns_cleanup($action, $tag, &$data) {
  return patterns_results();
}

// Return which callback functions to actually use.
function menu_patterns_callbacks($action, $tag, &$data) {
  $desc = menu_patterns();
  $result = $desc[$tag][$action];
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

/*function menu_patterns($op, $id = NULL, &$data = NULL, &$a = NULL) {
 switch ($op) {
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
 switch ($id) {
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
 $data['menu_name'] = 'menu-' . $data['menu_name'];
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
 if (isset($data['delete']) && empty($data['menu_name'])) {
 return t('"menu_name" is required.');
 }
 if (!empty($data['menu_name']) && preg_match('/[^a-z0-9\-]/', $data['menu_name'])) {
 return t('The menu name may only consist of lowercase letters, numbers, and hyphens: %menu', array('%menu' => $data['menu_name']));
 }
 break;
 case 'menu_item':
 if (empty($data['mlid'])) {
 if (empty($data['link_path'])) {
 return t('"path" is required.');
 }
 if (empty($data['link_title'])) {
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
 elseif ($id == 'menu_item') {
 if ($data['delete']) {
 return 'menu_item_delete_form';
 }
 //         elseif ($data['reset']) {
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
 elseif ($id == 'menu_reset_item_confirm') {
 // @TODO
 }
 elseif ($id == 'menu_edit_item') {
 $data['customized'] = 1;
 $data['module'] = 'menu';
 if ($data['disable']) {
 $data['enabled'] = 0;
 unset($data['disable']);
 }
 elseif ($data['enable']) {
 $data['enabled'] = 1;
 unset($data['disable']);
 }
 //  $d = array();
 //  $d['menu'] = $data;
 //  $data = $d;
 }
 return $data;
 break;

 // Validate the values for an action before running the pattern
 case 'validate':
 switch ($id) {
 case 'menu_edit_item':

 if (empty($data['menu_name']) && !empty($data['menu'])) {
 if ($count = db_query("SELECT COUNT(*) FROM {menu_custom} WHERE title = :menu", array('menu' => $data['menu']))->fetchField()) {
 if ($count == 1) {
 $data['menu_name'] = db_query("SELECT menu_name FROM {menu_custom} WHERE title = :menu", array('menu' => $data['menu']))->fetchField();
 unset($data['menu']);
 }
 else {
 return t("Menu name couldn't be determined accurately because more than one menu has exactly the same title. You need to provide menu_name instead of the title: '%title'", array('%title' => $data['menu']));
 }
 }
 }

 if ((empty($data['mlid']) || isset($data['parent'])) && empty($data['menu_name'])) {
 return t('"menu_name" is required.');
 }

 if ($data['parent'] == '0') {
 $data['parent'] = $data['menu_name'] . ':0';
 }
 elseif (is_numeric($data['parent']) && !empty($data['menu_name']) && db_query("SELECT mlid FROM {menu_links} WHERE menu_name = :menu AND mlid = :mlid", array('menu' => $data['menu_name'],'mlid'=>$data['parent']))->fetchField()) {
 $data['parent'] = $data['menu_name'] . ':' . $data['parent'];
 }
 // @TODO this may not work if there is more then one menu item with the same title within the same menu
 elseif (is_string($data['parent']) && !empty($data['menu_name']) && $plid = db_query("SELECT mlid FROM {menu_links} WHERE menu_name = :menu AND link_title = :title", array('menu' => $data['menu_name'],'title'=>$data['parent']))->fetchField()) {
 $data['parent'] = $data['menu_name'] . ':' . $plid;
 }
 elseif (empty($data['mlid']) && empty($data['parent']) && !empty($data['link_title']) && !empty($data['link_path'])) {
 $data['parent'] = $data['menu_name'] . ':0';
 }

 if (!empty($data['parent'])) {
 list($menu_name, $mlid) = explode(':', $data['parent']);
 if ($mlid != 0 && !db_query("SELECT count(*) FROM {menu_links} WHERE menu_name = :menu AND mlid = :mlid",array('menu' => $menu_name,'mlid'=>$mlid))->fetchField()) {
 return t('"parent" is not valid.');
 }
 }
 elseif (empty($data['mlid'])) {
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
 if (!$menu) {
 if (strpos($data['menu_name'], 'menu-') === 0) {
 // this ensures that same pattern can be re-run without getting "menu exists" error
 // $a is $form_state array that will be passed to drupal_form_submit()
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
 if (!$data['mlid']) {
 return array('add', NULL, menu_load($data['menu_name']));
 }
 else {
 return array('edit', menu_link_load($data['mlid']), NULL);
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
 elseif ($id == 'menu_edit_item') {
 $data = reset($data);
 if ($data['mlid']) {
 return $data['mlid'];
 }
 elseif ($mlid = db_query("SELECT mlid FROM {menu_links} WHERE menu_name = :menu AND link_path = :path AND link_title = :title ORDER BY mlid DESC", array('nemu'=>$data['menu_name'],'path'=> $data['link_path'], 'title'=>$data['link_title']))->fetchField()) {
 return $mlid;
 }
 }
 break;
 }
 }*/
