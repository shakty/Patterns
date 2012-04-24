<?php
/*
 * @file
 * Patterns component for users.
 * TODO: when creating a user, check if the email is already taken.
 * TODO: user modifying does not work, it's asking for the password.
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
    PATTERNS_EXPORT => array(PATTERNS_EXPORT_ALL => 'user_patterns_export_all_roles', ),
  );

  $actions['permission'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete Permissions'),
    PATTERNS_CREATE => array('user_admin_permissions'),
    PATTERNS_MODIFY => array('user_admin_permissions'),
    PATTERNS_DELETE => array('user_admin_permissions'),
    PATTERNS_FILES => $files,
    PATTERNS_EXPORT => array(PATTERNS_EXPORT_ALL => 'user_patterns_export_all_permissions', ),
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


function user_patterns_export_all_roles($args = NULL, &$result = NULL) {
  $roles = user_roles(TRUE);
  $result = array();
  foreach ($roles as $role) {
    $data = array('tag' => 'role');
    $data['value'] = $role;
    $action = array(PATTERNS_CREATE => $data);
    array_push($result, $action);
  }
  return $result;
}

function user_patterns_export_all_permissions($args = NULL, &$result = NULL) {
  $info = user_patterns();
  $form_id = current($info['permission'][PATTERNS_MODIFY]);
  $form_data = patterns_api_extract_actions($form_id, array(), 'permission', PATTERNS_MODIFY);
  $form_data = $form_data[0][PATTERNS_MODIFY];

  $roles = $form_data['role_names'];
  $result = array();
  foreach ($roles as $id => $role) {
    $perms = $form_data[$id-1]; // TODO: this might not be right if the IDs are not contiguous.
    $perms2 = array(); // Permissions that are set.
    foreach ($perms as $perm) {
      if ($perm !== 0) {
        array_push($perms2, $perm);
      }
    }
    $permissions = implode(', ', $perms2);
    $data = array('tag' => 'permission');
    $data['rid'] = $id;
    $data['value'] = $permissions;
    $action = array(PATTERNS_CREATE => $data);
    array_push($result, $action);
  }
  return $result;
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
  $status = PATTERNS_SUCCESS;
  $msg = t('Execution successful');
  if ($tag == 'user') {
    if (!isset($data['name'])) {
      if (!isset($data['uid'])) {
        $status = PATTERNS_ERR;
        $msg = t('Cannot create/modify user. Missing name and uid.');
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
    /* if (isset($data['value'])) {
      $data['name'] = $data['value'];
      unset($data['value']);
    } */
  }
  elseif ($tag == 'permission') {
    if (isset($data['role']) && !isset($data['rid'])) {
      $data['rid'] = $data['role'];
    }
    unset($data['role']);
  }
  return patterns_results($status, $msg);
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
    // $count = db_query("SELECT COUNT(*) FROM {users} WHERE uid = :uid", array('uid' => $data['uid']))->fetchField();
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
      $msg = t('Cannot delete super-user account.');
    }
    elseif ($data['uid'] == $user->uid && $action === PATTERNS_DELETE) {
      $status = PATTERNS_ERR;
      $msg = t('You cannot delete the current users account. Please log in with a different account and try again.');
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
    $isset_rid = FALSE;
    $isset_rname = FALSE;
    $rid_exist = FALSE;
    $rname_exist = FALSE;
    $rname_id = FALSE;

    if (isset($data['rid'])) {
      $isset_rid = TRUE;
    }
    if (isset($data['name'])) {
      $isset_rname = TRUE;
    }
    if ($isset_rid) {
      if (is_string($data['rid'])) {
        $rid = db_select('role', 'r')
          ->fields('r', array('rid'))
          ->condition('r.name', $data['rid'])
          ->execute()
          ->fetchField();
        if ($rid) {
          $data['rid'] = $rid;
          $rid_exist = TRUE;
        }
      }
      elseif (is_numeric($data['rid'])) {
        $cnt = db_query("SELECT COUNT(*) FROM {role} WHERE rid = :id", array('id' => $data['rid']))->fetchField(); // TODO: count()
        if ($cnt) {
          $rid_exist = TRUE;
        }
      }
    }
    if ($isset_rname) {
      $rid = db_select('role', 'r')
        ->fields('r', array('rid'))
        ->condition('r.name', $data['name'])
        ->execute()
        ->fetchField();
      if ($rid) {
        $rname_exist = TRUE;
        $rname_id = $rid;
      }
    }

    if (($rid_exist && ($data['rid'] == DRUPAL_ANONYMOUS_RID || $data['rid'] == DRUPAL_AUTHENTICATED_RID))
            || ($rname_exist && ($data['name'] == DRUPAL_ANONYMOUS_RID || $data['name'] == DRUPAL_AUTHENTICATED_RID))) {
      $status = PATTERNS_ERR;
      $msg = t('You cannot manipulate authenticated or anonymous roles.', array('%role' => $data['name']));
    }
    elseif ($action === PATTERNS_CREATE) {
      if (!$isset_rname) {
        $status = PATTERNS_ERR;
        $msg = t('You must specify a name to create role.');
      }
      elseif ($rname_exist) {
        $status = PATTERNS_ERR;
        $msg = t('The specified role name %name already exists.', array('%name' => $data['name']));
      }
      elseif ($isset_rid) {
        $status = PATTERNS_ERR;
        $msg = t('You cannot create a role with fixed rid.', array('%role' => $data['name']));
      }
    }
    elseif ($action === PATTERNS_MODIFY) {
      if (!$isset_rid || !$isset_rname) {
        $status = PATTERNS_ERR;
        $msg = t('You must specify a name and rid to modify a role.', array('%role' => $data['name']));
      }
      elseif (!$rid_exist) {
        $status = PATTERNS_ERR;
        $msg = t('The given role id %rid does not exist.', array('%rid' => $data['rid']));
      }
      elseif ($rname_exist) {
        if ($data['rid'] == $rname_id) {
          $status = PATTERNS_ERR;
          $msg = t('It is unnecesarry to rename a role to itself.');
        }
        else {
          $status = PATTERNS_ERR;
          $msg = t('The given role name %rname already exists.', array('%rname' => $data['name']));
        }
      }
    }
    elseif ($action === PATTERNS_DELETE) {
      if (!$isset_rid && !$isset_rname) {
        $status = PATTERNS_ERR;
        $msg = t('You must specify a rid or name to delete.');
      }
      elseif ($isset_rid && !$rid_exist) {
        $status = PATTERNS_ERR;
        $msg = t('The given role id %rid does not exist.', array('%rid' => $data['rid']));
      }
      elseif ($isset_rname) {
        if (!$rname_exist) {
          $status = PATTERNS_ERR;
          $msg = t('The given role name %rname does not exist.', array('%rname' => $data['name']));
        }
        else {
          $data['rid'] = $rname_id;
        }
      }
    }
  }
  elseif ($tag == 'permission') {
    $role_name = FALSE;
    if (isset($data['rid'])) {
      // If rid is a string then we get the id.
      if (is_string($data['rid'])) {
        $rid = db_select('role', 'r')
          ->fields('r', array('rid'))
          ->condition('name', $data['rid'])
          ->execute()->fetchField();
        //print_r('rid: ' . ($rid ? $rid : 'false'));
        //var_dump($rid);
        if (!$rid) {
          //print_r('rid is FALSE');
          $status = PATTERNS_ERR;
          $msg = t('Role name %name does not exist.', array('%name' => $data['rid']));
        }
        else {
          $role_name = $data['rid'];
          $data['rid'] = $rid;
        }
        //die;
      }
      // If rid is a number then we check if role exists.
      elseif (is_numeric($data['rid'])) {
        $exist = db_query("SELECT COUNT(*) FROM {role} WHERE rid = :rid", array('rid' => $data['rid']))->fetchField(); // TODO: count()
        if (!$exist) {
          $status = PATTERNS_ERR;
          $msg = t('Role id %id does not exist.', array('%id' => $data['rid']));
        }
        else {
          $role_name = db_query('SELECT name FROM {role} WHERE rid = :rid', array('rid' => $data['rid']))->fetchField();
        }
      }
      // If role not does exist then throw error.
      else {
        $status = PATTERNS_ERR;
        $msg = t('Role id %id must be a number or a string.', array('%id' => $data['rid']));
      }

      // Getting the specified permission (value) list.
      if ($role_name !== FALSE) {
        $permissions = FALSE;
        $valid_format = TRUE;

        if (!empty($data['value'])) {
          $permissions = $data['value'];
        }
        // Checking if format is valid.
        if ($permissions !== FALSE) {
          if (gettype($permissions) != 'array') {
            $valid_format = FALSE;
          }
          else {
            foreach ($permissions as $perm) {
              if (gettype($perm) != 'string') {
                $valid_format = FALSE;
                break;
              }
            }
          }
        }
        if (!$valid_format) {
          $status = PATTERNS_ERR;
          $msg = t('The value format is invalid, it must be a list if it is set.');
        }
        else {
          // Getting all permissions of the system and to the given role.
          $all_permission = user_permission_get_modules();
          $role_permissions = user_role_permissions(array($data['rid'] => $role_name));
          $role_permissions = $role_permissions[$data['rid']];

          // At create or delete the value must be set.
          if (($action == PATTERNS_CREATE || $action == PATTERNS_DELETE) && $permissions == FALSE) {
            $status = PATTERNS_ERR;
            $msg = t('Permission list cannot be empty.');
          }
          // If permissions are set we have to check if each of them are exists.
          elseif ($permissions !== FALSE) {
            $bad_permissions = array();
            $good_permissions = array();
            foreach ($permissions as $perm) {
              if (!array_key_exists($perm, $all_permission)) {
                $bad_permissions[] = $perm;
              }
              else {
                $good_permissions[] = $perm;
              }
            }
            if (count($bad_permissions) > 0) {
              $list = '';
              $i = 0;
              foreach ($bad_permissions as $p) {
                if ($i > 0) $list .= ', ';
                $list .= $p;
                $i++;
              }
              $status = PATTERNS_WARN;
              $msg = t('The permissions listed here do not exist: %perms', array('%perms' => $list));
            }
            $good_perms_string = '';
            $i = 0;
            foreach ($good_permissions as $gp) {
              if ($i > 0) $good_perms_string .= ', ';
              $good_perms_string .= $gp;
              $i++;
            }
          }
          if ($action == PATTERNS_CREATE) {
            $data['value'] = $good_perms_string;
            foreach ($role_permissions as $rp => $in) {
              $data['value'] .= ', ' . $rp;
            }
          }
          elseif ($action == PATTERNS_MODIFY) {
            $data['value'] = $good_perms_string;
          }
          elseif ($action == PATTERNS_DELETE) {
            foreach ($good_permissions as $gp) {
              if (array_key_exists($gp, $role_permissions)) {
                unset($role_permissions[$gp]);
              }
            }
            $data['value'] = '';
            $i = 0;
            foreach ($role_permissions as $rp => $in) {
              if ($i > 0) $data['value'] .= ', ';
              $data['value'] .= $rp;
              $i++;
            }
          }
        }
      }
    }
    else {
      $status = PATTERNS_ERR;
      $msg = t('You must specify a role id or role name to apply permissions to.');
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
  elseif ($tag == 'permission') {
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

    if (is_string($data['value'])) {
      $p = explode(',', $data['value']);
      unset($data['value']);
      foreach ($p as $key => $value) {
        $val = trim(trim($value), '\'"');
        if (in_array($val, $permissions)) {
          //if ($action === PATTERNS_DELETE) {
            //$perms[$val] = NULL;
          //}
          //else {
            $perms[$val] = $val;
          //}
        }
      }
      $data[$data['rid']] = $perms;
    }
    else {
      for ($i = 0; $item = $data[$i]; $i++) {
        if (in_array($item, $permissions)) {
          if ($action === PATTERNS_DELETE) {
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
/*
 * if ($id == 'user_admin_permissions') {
 *   if (is_numeric($data['rid'])) {
 *     $rid = $data['rid'];
 *   }
 *   elseif (is_string($data['rid'])) {
 *     $rid = db_query('SELECT rid FROM {role} WHERE name = :name', array('name' => $data['rid']))->fetchField();
 *   }
 *
 *   if (!db_query("SELECT COUNT(*) FROM {role} WHERE rid = :rid", array('rid' => $rid))->fetchField()) {
 *     return t('Invalid role %role to set permissions for.', array('%role' => $data['role'] ? $data['role'] : $data['rid']));
 *   }
 * }
 */
  $result = $data;

  return patterns_results($status, $msg, $result);

}

// Build a patterns actions and parameters
function user_patterns_params($action, $form_id, &$data = NULL, &$a) {
  if ($form_id == 'user_cancel_confirm_form' && $form_id == 'user_profile_form') {
    $result = array(user_load($data['uid']));
  }
  elseif ($form_id == 'user_admin_role_delete_confirm' || $form_id == 'user_admin_role') {
    $result = array((object) $data);
  }
  elseif ($form_id == 'user_admin_permissions') {
    $result = array($data['rid']);
  }
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Cleanup any global settings after the action runs
function user_patterns_cleanup($action, $tag, &$data) {
  return patterns_results();
}
