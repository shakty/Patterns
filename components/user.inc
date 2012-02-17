<?php
/*
 * @file
 * Patterns component for users.
 */

/**
 * Implements hook_patterns() for the user module.
 */
function user_patterns($data) {

  $files = array('modules/user/user.admin.inc', 'modules/user/user.pages.inc');

  $actions['user'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete Users'),
    PATTERNS_CREATE => array('user_register_form'),
    PATTERNS_MODIFY => array('user_profile_form'),
    PATTERNS_DELETE => array('user_cancel_confirm_form'),
    PATTERNS_FILES => $files,
  );

  $actions['role'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete Roles'),
    PATTERNS_CREATE => array('user_admin_roles'),
    PATTERNS_MODIFY => array('user_admin_role'),
    PATTERNS_DELETE => array('user_admin_role_delete_confirm'),
    PATTERNS_FILES => $files,
  );

  $actions['permission'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete Permissions'),
    PATTERNS_CREATE => array('user_admin_permissions'),
    PATTERNS_MODIFY => array('user_admin_permissions'),
    PATTERNS_DELETE => array('user_admin_permissions'),
    PATTERNS_FILES => $files,
  );

  return $actions;

}

function user_patterns_actions($data) {
  return array(
    'user_register_form' => t('User: Add user with the Register Form'),
    'user_profile_form' => t('User: Edit user'),
    'user_cancel_confirm_form' => t('User: Delete user'),
    'user_admin_roles' => t('User: Add role'),
    'user_admin_role' => t('User: Edit role'),
    'user_admin_role_delete_confirm' => t('User: Delete role'),
    'user_admin_permissions' => t('User: Configure permissions'),
  );
}

// returns arguments needed for the forms when processing automatically
// $loop is set if we want to extract all values
function user_patterns_get_arguments($action, $tag, $form_id, &$loop = FALSE) {
  $return = array();
  $loop = TRUE;
  if ($form_id == 'user_profile_form') {
    $query = db_select('users', 'u');
    $query->condition('u.uid', 0, '<>');
    $query = $query->extend('PagerDefault')->extend('TableSort');
    $query->fields('u', array('uid'));
    $result = $query->execute()->fetchAll();
    foreach ($result as $account) {
      $user = user_load($account->uid);
      array_push($return, array($user));
    }
  }
  elseif ($form_id == 'user_admin_role') {
    $roles = user_roles();
    foreach ($roles as $rid => $name) {
      if (!($rid == DRUPAL_ANONYMOUS_RID || $rid == DRUPAL_AUTHENTICATED_RID)) {
        array_push($return, array((object) array('rid' => $rid, 'name' => $name, 'weight' => 0)));
      }
    }
  }
  elseif ($form_id == 'user_admin_permissions') {
    $roles = user_roles();
    foreach ($roles as $rid => $name) {
      if (!($rid == DRUPAL_ANONYMOUS_RID || $rid == DRUPAL_AUTHENTICATED_RID)) {
        array_push($return, array($rid));
      }
    }
  }
  return $return;
}
/**
 * Implements hook_patterns_prepare() for the user module.
 */
function user_patterns_prepare($action, $tag, &$data) {
  if ($tag == 'user') {
    // Check with isset first, otherwise we get a notice
    if (!isset($data['name'])) {
      if (!isset($data['uid'])) {
        return t('Cannot create/modify user. Missing name and uid.');
      }
      // Retrieve the username name from the db
      $data['name'] = db_select('users', 'u')
                        ->fields('u', array('name'))
                        ->condition('u.uid', $data['uid'])
                        ->execute()
                        ->fetchField();
    }
    else {
      if (!isset($data['uid'])) {
        $data['uid'] = db_select('users', 'u')
                        ->fields('u', array('uid'))
                        ->condition('u.name', $data['name'])
                        ->execute()
                        ->fetchField();
      }
    }

    // @TODO: Check if we are overriding the password
    if (isset($data['password'])) {
      $data['pass'] = array(
        'pass1' => $data['password'],
        'pass2' => $data['password'],
      );
      unset($data['password']);
    }

    if (isset($data['timezone'])) {
    /* TODO: this needs work and verification
      if (in_array($data['timezone'], array('0', '+0', '-0', '0000', '-0000'))) {
        $data['timezone'] = '+0000';
      }
      if (!empty($data['timezone']) && preg_match('/(\+|-)([0][0-9]|[1][01])(00|30|45)/', $data['timezone'], $parts)) {
        $seconds = $parts[2] * 3600 + $parts[3] * 60;
        $sign = $parts[1] == '+' ? '' : '-';
        $data['timezone'] = $sign . $seconds;
      }
    */
    }
    else {
      // Default timezone is Europe/Berlin.
      $data['timezone'] = 'Europe/Berlin';
    }
  }
  elseif ($tag == 'role') {
    if (isset($data['value'])) {
      $data['name'] = $data['value'];
      unset($data['value']);
    }

    if (empty($data['rid'])) {
      $rid = db_select('role', 'r')
               ->fields('r', array('rid'))
               ->condition('r.name', $data['name'])
               ->execute()
               ->fetchField();
//      $rid = db_query("SELECT rid FROM {role} WHERE name = :name", array('name' => $data['name']))->fetchField();
      if ($rid) {
        $data['rid'] = $rid;
      }
    }
  }
  elseif ($tag == 'permissions') {
    if (isset($data['role']) && !$data['rid']) {
      $data['rid'] = $data['role'];
    }
    unset($data['role']);
  }
  return patterns_results();
}

// Validate the values for an action before running the pattern
function user_patterns_validate($action, $tag, &$data) {
  $status = PATTERNS_SUCCESS;
  $msg = '';

  if ($tag == 'user') {
    // Make sure uid is valid.
    $count = db_select('users', 'u')
               ->condition('u.uid', $data['uid'])
               ->countQuery()
               ->execute()
               ->fetchField();
    //$count = db_query("SELECT COUNT(*) FROM {users} WHERE uid = :uid", array('uid' => $data['uid']))->fetchField();
    if (!empty($data['uid']) && !$count) {
      $status = PATTERNS_ERR;
      $msg = t('Invalid user ID supplied: %uid', array('%uid' => $data['uid']));
    }

    // TODO: check $data['method'] for valid values if PATTERNS_DELETE.

    // Cannot create/edit/delete anon user.
    global $user; // Current logged in user.

    if ($data['uid'] === 0) {
      $status = PATTERNS_ERR;
      $msg = t('Cannot modify anonymous user account.');
    }
    elseif ($data['uid'] == 1 && $action === PATTERNS_DELETE) {
      $status = PATTERNS_ERR;
      $msg = t('Cannot delete super-user account. Please modify pattern and try again.');
    }
    elseif ($data['uid'] == $user->uid && $action === PATTERNS_DELETE) {
      $status = PATTERNS_ERR;
      $msg = t('You cannot delete the current users account. Please login with a different account and try again.');
    }
    elseif ($data['uid'] && $action === PATTERNS_CREATE) {
      $status = PATTERNS_ERR;
      $msg = t('The user %name already exist.', array('%name' => $data['name']));
    }
    elseif (empty($data['uid']) && (empty($data['name']) || empty($data['mail']))) {
      $status = PATTERNS_ERR;
      $msg = t("<br/>1. you didn't supply enough data for creation of the new account (both name and mail are required)<br/>or<br/>2. account you are trying to update doesn't exist (invalid uid or name).");
    }

    if (!empty($data['timezone'])) {
      $zones = system_time_zones();
      if (!array_key_exists($data['timezone'], $zones)) {
        $status = PATTERNS_ERR;
        $msg = t('Invalid timezone: %zone. Use the following format: America/New_York or Europe/Berlin', array('%zone' => $data['timezone']));
      }
    }
  }

  elseif ($tag == 'role') {
    if (isset($data['rid']) && (DRUPAL_ANONYMOUS_RID == $data['rid'] || DRUPAL_AUTHENTICATED_RID == $data['rid'])) {
      $status = PATTERNS_ERR;
      $msg = t('The authenticated or anonymous roles cannot be updated or deleted.');
    }
    elseif (!empty($data['rid']) && !db_query("SELECT COUNT(*) FROM {role} WHERE rid = :id", array('id' => $data['rid']))->fetchField()) {
      $status = PATTERNS_ERR;
      $msg = t('Invalid role id: %rid.', array('%rid' => $data['rid']));
    }
    elseif ($action === PATTERNS_CREATE && !empty($data['rid'])) {
      $status = PATTERNS_ERR;
      $msg = t('You cannot create a role with fixed rid id');
    }
  }
  elseif ($tag == 'permissions') {
    if (!$data['rid']) {
      $status = PATTERNS_ERR;
      $msg = t('You must specify a role to apply permissions to.');
    }
  }

  return patterns_results($status, $msg);
}

// Return which callback functions to actually use.
function user_patterns_callbacks($action, $tag, &$data) {

  if ($tag == 'user') {
    // Check if the user needs updating.
    if ($data['uid']) {
      if ($action === PATTERNS_DELETE) {
        $result = array('user_cancel_confirm_form');
      }
      elseif ($action === PATTERNS_MODIFY) {
        $result = array('user_profile_form');
      }
    }
    else {
      if ($action !== PATTERNS_DELETE) {
        $result = array('user_register_form');
      }
      else {
        // TODO: ???
      }
    }
  }
  elseif ($tag == 'role') {
    // Role names should always be unique.
    if ($data['rid'] && $data['name'] && $action === PATTERNS_MODIFY) {
      $result = array('user_admin_role');
    }
    elseif ($action === PATTERNS_DELETE) {
      $result = array('user_admin_role_delete_confirm');
    }
    elseif ($action === PATTERNS_CREATE) {
      $result = array('user_admin_roles');
    }
  }
  elseif ($tag == 'permissions') {
    $result = array('user_admin_permissions');
  }
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Prepare for valid processing of this type of component
function user_patterns_build($action, $form_id, &$data = NULL, &$a) {
  $status = PATTERNS_SUCCESS;
  $msg = '';
  $result = NULL;

  if ($form_id == 'user_register_form') {
    if (isset($data['uid'])) {
      unset($data['uid']);
    }

    // Ste: added
//        if (!isset($data['user'])) {
//          $data['user'] = (object) $data;
//        }

  }
  elseif ($form_id == 'user_profile_form') {
    // Make sure that required fields are not left empty.
    if (empty($data['mail'])) {
      $data['mail'] = db_query("SELECT mail FROM {users} WHERE uid = :uid", array('uid' => $data['uid']))->fetchField();
    }
    if (empty($data['name'])) {
      $data['name'] = db_query("SELECT name FROM {users} WHERE uid = :uid", array('uid' => $data['uid']))->fetchField();
    }

    static $old_q; // TODO: What is this?
    $old_q = $_GET['q'];
    $_GET['q'] = 'user/' . $data['uid'] . '/edit';
  }
  elseif ($id == 'user_cancel_confirm_form') {
    $data['confirm'] = 1;
    switch ($data['method']) {
      case '1':
        $data['user_cancel_method'] = 'user_cancel_block';
        break;
      case '2':
        $data['user_cancel_method'] = 'user_cancel_block_unpublish';
        break;
      case '3':
        $data['user_cancel_method'] = 'user_cancel_reassign';
        break;
      case '4':
        $data['user_cancel_method'] = 'user_cancel_delete';
        break;
    }
  }
  elseif ($form_id == 'user_admin_roles' || $form_id == 'user_admin_role_delete_confirm' || $form_id == 'user_admin_role') {
    static $old_q; // TODO: What is this?
    $old_q = $_GET['q'];
    if ($data['rid']) {
      if ($data['delete']) {
        $data['confirm'] = 1;
        $_GET['q'] = 'admin/people/permissions/roles/delete/' . $data['rid'];
      }
      else {
        $_GET['q'] = 'admin/people/permissions/roles/edit/' . $data['rid'];
      }
    }
    else {
      $_GET['q'] = '';
    }

    if ($action === PATTERNS_DELETE) {
      $data['op'] = t('Delete');
    }
    elseif ($data['rid']) {
      $data['op'] = t('Save role');
    }
    else {
      $data['op'] = t('Add role');
    }
  }
  elseif ($form_id == 'user_admin_permissions') {
    // TODO: use user_permission_get_modules() instead.
    // Get list of permissions to ensure only available permissions are saved.
    $permissions = array();
    foreach (module_list(FALSE, FALSE, TRUE) as $module) {
      if ($p = module_invoke($module, 'permission')) {
        $permissions = array_merge($permissions, array_keys($p));
      }
    }

    if (!empty($data['overwrite'])) {
      foreach ($permissions as $p) {
        $perms[$p] = NULL;
      }
    }

    if (is_string($data['value'])) {
      $p = explode(',', $data['value']);
      unset($data['value']);
      foreach ($p as $key => $value) {
        $val = trim(trim($value), '\'"');
        if (in_array($val, $permissions)) {
          if (!empty($data['delete'])) {
            $perms[$val] = NULL;
          }
          else {
            $perms[$val] = $val;
          }
        }
      }
      $data[$data['rid']] = $perms;
    }
    else {
      for ($i = 0; $item = $data[$i]; $i++) {
        if (in_array($item, $permissions)) {
          if (!empty($data['delete'])) {
            $perms[$item] = 0;
          }
          else {
            $perms[$item] = $item;
          }
        }
        unset($data[$i]);
      }
      $data[$data['rid']] = $perms;
    }

    module_load_include('inc', 'user', 'user.admin'); // TODO: Isn't this already loaded?
    if (is_numeric($data['rid'])) {
      $rid = $data['rid'];
    }
    elseif (is_string($data['rid'])) {
      $rid = db_query('SELECT rid FROM {role} WHERE name = :name', array('name' => $data['rid']))->fetchField();
      $role = $data['rid'];
      $data[$rid] = $data[$role];
      $data['rid'] = $rid;
      unset($data[$role]);
    }
    $data['op'] = t('Save permissions');
  }

  // TODO: check if these validations work
  if ($id == 'user_admin_permissions') {
    if (is_numeric($data['rid'])) {
      $rid = $data['rid'];
    }
    elseif (is_string($data['rid'])) {
      $rid = db_query('SELECT rid FROM {role} WHERE name = :name', array('name' => $data['rid']))->fetchField();
    }

    if (!db_query("SELECT COUNT(*) FROM {role} WHERE rid = :rid", array('rid' => $rid))->fetchField()) {
      return t('Invalid role %role to set permissions for.', array('%role' => $data['role'] ? $data['role'] : $data['rid']));
    }
  }
  $result = $data;

  return patterns_results($status, $msg, $result);
}

// Build a patterns actions and parameters
function user_patterns_params($action, $form_id, &$data = NULL, &$a) {
  if ($form_id == 'user_cancel_confirm_form' && $form_id == 'user_profile_form') {
    $result = array(user_load($data['uid']));
  }
  elseif ($form_id == 'user_admin_role_delete_confirm' || $form_id == 'user_admin_role') {
    $result = (object) $data; // TODO: array?
  }
  elseif ($form_id == 'user_admin_permissions') {
    $result = $data['rid']; // TODO: array?
  }
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Cleanup any global settings after the action runs
function user_patterns_cleanup($action, $tag, &$data) {
  return patterns_results();
}
