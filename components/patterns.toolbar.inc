<?php
/*
 * @file
 * Patterns component for toolbar.
 */

/**
 * hook_patterns()
 */
function toolbar_patterns() {
  $files = array('modules/user/user.admin.inc');
  $actions['toolbarrole'] = array(
    PATTERNS_INFO => t('deside who can see the toolbar'),
    PATTERNS_MODIFY => array('user_admin_permissions'),
    PATTERNS_FILES => $files,
  );
  return $actions;
}

function toolbar_patterns_actions() {
  return array(
    'user_admin_permissions' => t('deside who can use toolbar'),
    );
}

/**
 * hook_patterns_prepart()
 */
function toolbar_patterns_prepare($action, $tag, &$data = NULL) {
  $rids = array();
  $roles = array();
  //get the rid from role's name for add 'access toolbar' to role.
  if (isset($data['addroles'])) {
    $p = explode(',', $data['addroles']);
    unset($data['addroles']);
    foreach ($p as $key => $value) {
      $value = trim(trim($value), '\'"');
      $rid = db_select('role', 'r')
             ->fields('r', array('rid'))
             ->condition('r.name',$value) 
             ->execute()
             ->fetchField();
      $rids[$rid] = $rid;
      $roles[$rid] = $value;
    }
  }
  //get the rid from role's name for delete 'access toolbar' from role.
  if (isset($data['delroles'])) {
    $pdel = explode(',', $data['delroles']);
    unset($data['delroles']);
    foreach ($pdel as $key => $value) {
      $value = trim(trim($value), '\'"');
      $rid = db_select('role', 'r')
             ->fields('r', array('rid'))
             ->condition('r.name',$value) 
             ->execute()
             ->fetchField();
      $rids[$rid] = NULL;
      $roles[$rid] = NULL;
    }
  }
  $data['rid'] = $rids;
  $data['role'] = $roles;
  return patterns_results();
}

/**
 * hook_patterns_validate()
 */
function toolbar_patterns_validate($action, $tag, &$data = NULL) {
  $status = PATTERNS_SUCCESS;
  $msg = '';
  if (empty($data['rid'])) {
    $status = PATTERNS_ERR;
    $msg = t('there is no available role.');
  }
  $rid = db_select('role', 'r')
         ->fields('r', array('rid'))
         ->condition('r.name','administrator') 
         ->execute()
         ->fetchField();
  foreach ($data['rid'] as $key => $value) {
    if ($rid == $value) {
      $status = PATTERNS_ERR;
      $msg = t('administrator should keep static.');
    }
  }
  
  foreach ($data['role'] as $key => $value) {
    if ($value == NULL) {
      $riddel = db_select('role_permission', 'r')
             ->fields('r', array('rid','permission'))
             ->condition('r.rid', $key) 
             ->condition('r.permission', 'access toolbar')
             ->execute()
             ->fetchField();
      if ($riddel == NULL) {
        $status = PATTERNS_ERR;
        $msg = t('delete wrong role.');
      }
    } else {
      $ridadd = db_select('role_permission', 'r')
             ->fields('r', array('rid','permission'))
             ->condition('r.rid', $key) 
             ->condition('r.permission', 'access toolbar')
             ->execute()
             ->fetchField();
      if ($ridadd != NULL) {
        $status = PATTERNS_ERR;
        $msg = t('add wrong role.(the role has the access already.)');
      }
    }
  }
  return patterns_results($status, $msg);
}
    
/**
 * hook_patterns_callbacks()
 */
function toolbar_patterns_callbacks($action, $tag, &$data = NULL) {
  //$result = array('user_admin_permissions');
  $result = array('toolbar_role_modify');
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

/*
 * callbacks function.
 */
function toolbar_role_modify ($action, &$data) {
  foreach ($data['rid'] as $key => $value) {
    if ($value == NULL) {
      $and = db_and()->condition('rid', $key)->condition('permission', 'access toolbar');
      $qdel = db_delete('role_permission')
              ->condition($and)
              ->execute();
    } else {
      $query = db_insert('role_permission')
              ->fields(array(
                'rid' => $key,
                'permission' => 'access toolbar',
                'module' => 'toolbar',
                ))
              ->execute();
    }
  }
}

/**
 * the form method have some problems, so I use the callback function to operate on the database directly.
 */
/*
function toolbar_patterns_build($action, $form_id, &$data = NULL, &$a) {
  foreach ($data['role'] as $key => $value) {
    $data[$value] = 'access toolbar';
  }
  module_load_include('inc', 'user', 'user.admin'); 
  $data['op'] = t('Save permissions');
  $result = $data;
  return patterns_results($status, $msg, $result);
}
*/
