<?php
/**
 * @file
 * Patterns component for shortcut.
 *
 * there are seven form in the shortcut moudle.they can be seprated in three kinds:set, link, switch for user.
 * 
 * set(three form):
 *     add a shortcut set:just give a name for the new shortcut set.
 *     delete a shortcut set:just supply a shortcut set's name.
 *     rename a shortcut set:need two name, one is used to define tht shortcut set which will be modified, the other is the new name.
 * 
 * link(three form):
 *     add a shortcut link: supply the name of a shortcut set which the new link will be in,supply the link's title and path.
 *     delete a shortcut link: just give the link's id, the id is in the menu_links table of the database.
 *     edit a shortcut link: need the link's id, need the link's new title or/and new path.
 *
 * switch for user(one form);
 *     need a user's name
 *     need a shortcut set'name
 *     the user will use the shortcut set.
 */

/**
 * hook_patterns()
 */
function shortcut_patterns() {
  $files = array('modules/shortcut/shortcut.admin.inc');
  
  $actions['shortcut_set'] = array(
    PATTERNS_INFO => t('create/delete/modify shortcut set'),
    PATTERNS_CREATE => array('shortcut_set_add_form'),
    PATTERNS_MODIFY => array('shortcut_set_edit_form'),
    PATTERNS_DELETE => array('shortcut_set_delete_form'),
    PATTERNS_FILES => $files,
    );
  $actions['shortcut_link'] = array(
    PATTERNS_INFO => t('add a new link to one shortcut_set'),
    PATTERNS_CREATE => array('shortcut_link_add'),
    PATTERNS_MODIFY => array('shortcut_link_edit'),
    PATTERNS_DELETE => array('shortcut_link_delete'),
    PATTERNS_FILES => $files,
    );
  $actions['shortcut_set_user'] = array(
    PATTERNS_INFO => t('one user get one shortcut_set'),
    PATTERNS_MODIFY => array('shortcut_set_switch'),
    PATTERNS_FILES => $files,
    );
  
  return $actions;
}

function shortcut_patterns_actions() {
  return array(
    'shortcut_set_add_form' => t('Shortcut: add a new shortcut set'),
    'shortcut_set_edit_form' => t('Shortcut: change the name a shortcut set'),
    'shortcut_set_delete_form' => t('Shortcut: delete a shortcut set'),
    'shortcut_set_switch' => t('Shortcut: switch shortcut sets for user'),
    'shortcut_link_add' => t('Shortcut: add link to a shortcut set'),
    'shortcut_link_edit' => t('Shortcut: edit a shortcut link'),
    'shortcut_link_delete' => t('Shortcut: delete a shortcut link'),
    );
}

/**
 * hook_patterns_prepare()
 */
function shortcut_patterns_prepare($action, $tag, &$data = NULL) {
  // for set.
  if ($tag == 'shortcut_set') {
    if ($action == PATTERNS_CREATE ) {
      $data['new'] = $data['shortcut_set_name'];
    }
    if ($action == PATTERNS_MODIFY ) {
      $data['title'] = $data['newname'];
      $data['searchtitle'] = $data['oldname'];
    }
    if ($action == PATTERNS_DELETE) {
      $data['searchtitle'] = $data['name'];
    }
  }

  // for switch user
  if ($tag == 'shortcut_set_user') {
    $data['searchtitle'] = $data['name'];
    $data['account'] = NULL;
  }

  // for link
  if ($tag == 'shortcut_link' ) {
    if( $action == PATTERNS_CREATE) {
      $data['searchtitle'] = $data['shortcut_set_name'];
      $data['shortcut_link']['link_title'] = $data['shortcut_link_title'];
      $data['shortcut_link']['link_path'] = $data['shortcut_link_path'];
    }
    if( $action == PATTERNS_MODIFY) {
      $data['shortcut_link']['link_title'] = $data['new_link_title'];
      $data['shortcut_link']['link_path'] = $data['new_link_path'];
    }
  }
      
  return patterns_results();
}

/**
 * hook_patterns_validate()
 */
function shortcut_patterns_validate($action, $tag, &$data = NULL) {

  $status = PATTERNS_SUCCESS;
  $msg = '';

  //for link's edit and delete.
  //these two form have the third parm:$shortcut_link, follow code provide the parm.
  if( $tag == 'shortcut_link' ) {
    if( $action == PATTERNS_MODIFY || $action == PATTERNS_DELETE ) {
      $shortcutlink = db_select('menu_links','ml')
        //->fields('ml', array('menu_name','link_title', 'link_path', 'mlid'))
        ->fields('ml')
        ->condition('mlid', $data['menu_link_id'])
        ->execute()
        ->fetchAssoc();
        //follow line is for test
        //drupal_set_message(t('the %acc is %mlid  dfldjdkfdld',array('%acc' =>$shortcutlink['link_title'],'%mlid' => $shortcutlink['mlid'])));
      $data['shortcut_link_parm'] = $shortcutlink;
    }
  }
    
  //for set's edit&delete and link's add and switch user.
  //these three form(switch user just use the $set) have the third parm:$shortcut_set, follow code provide the parm.
  if(( $tag == 'shortcut_set' && $action != PATTERNS_CREATE ) || ($tag == 'shortcut_link' && $action == PATTERNS_CREATE) || ($tag == 'shortcut_set_user')) {
    $set = db_select('shortcut_set', 'ss')
      ->fields('ss',array('set_name'))
      ->condition('title', $data['searchtitle'])
      ->execute()
      ->fetchField();
    if ($set == FALSE) {
      $status = PATTERNS_ERR;
      $msg = 'Shortcut Error:there is no such name shortcut set';
      return patterns_results($status, $msg);
    } else {
      module_load_include('module', 'shortcut');
      $shortcutsetobj = shortcut_set_load($set);
      $data['shortcut_set_obj'] = $shortcutsetobj;
      if( $tag == 'shortcut_set_user') {
        $data['set'] = $set;
        $data['new'] = $data['name'];
      }
    } 
    if ($tag =='shortcut_set_user') {
      //this form have the third parm:$account, follow code provide it.
      if(isset($data['user'])) {
        module_load_include('module', 'user');
        if(($data['account'] = user_load_by_name( $data['user'])) == FALSE) {
          $status = PATTERNS_ERR;
          $msg = 'Shortcut Error:there is no such a user.';
          return patterns_results($status, $msg);
        }
      }
    }

  }
    
  return patterns_results($status, $msg);
}

/**
 * hook_patterns_callbacks()
 */
function shortcut_patterns_callbacks($action, $tag, &$data = NULL) {
  $allform = shortcut_patterns();
  $result = $allform[$tag][$action];
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful(in callbacks function)'), $result);
}


/**
 * hook_patterns_build()
 */
function shortcut_patterns_build($action, $form_id, &$data = NULL, &$a) {
  module_load_include('inc', 'shortcut', 'shortcut.admin');
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful(in build function)'), $data);
}

/**
 * hook_patterns_params()
 */
function shortcut_patterns_params($action, $form_id, &$data = NULL, &$a) {
  //provide the third parm.
  switch ($form_id) {
    case 'shortcut_set_edit_form':
    case 'shortcut_set_delete_form':
    case 'shortcut_link_add':
      $result = array( $data['shortcut_set_obj'] ); 
      break;
    case 'shortcut_set_switch':
      $result = array( $data['account'] );
      break;
    case 'shortcut_link_delete':
    case 'shortcut_link_edit':
      $result = array( $data['shortcut_link_parm']);
      break;
  }
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

/**
 * hook_patterns_cleanup()
 */
function shortcut_patterns_cleanup($action, $tag, &$data = NULL) {
  return patterns_results();
}
