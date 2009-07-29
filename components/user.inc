<?php

function user_patterns($op, $id = null, &$data = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('user', 'role', 'permissions');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'user_register' => t('User: Add user'),
        'user_profile_form' => t('User: Edit user'),
        'user_confirm_delete' => t('User: Delete user'),
        'user_admin_role' => t('User: Add, edit or remove role'),
        'user_admin_perm' => t('User: Configure permissions')
      );
    break;

    // Return a summary of an action
    case 'summary':
      if ($data['uid'] && !$data['name']) {
        $data['name'] = db_result(db_query('SELECT name FROM {users} WHERE uid = "%d"', $data['name']));
      }
      $variables = array('%user' => $data['name'], '%uid' => $data['uid'], '%role' => $data['name']);
      switch($id) {
        case 'user_register':
          return t('Create user %user', $variables);
        break;
        case 'user_profile_form':
          return t('Edit user account %user', $variables);
        break;
        case 'user_confirm_delete':
          return t('Delete user account %user', $variables);
        break;
        case 'user_admin_role':
          return t('Create user role %role', $variables);
        break;
        case 'user_admin_perm':
          return t('Add permissions for %role', $variables);
        break;
      }
    break;

    // Prepare data for processing
    case 'prepare':
      if ($id == 'user') {
        if ($data['name'] && !$data['uid']) {
          $data['uid'] = db_result(db_query('SELECT uid FROM {users} WHERE name = "%s"', $data['name']));
        }

        if ($data['password']) {
          $data['pass'] = array('pass1' => $data['password'], 'pass2' => $data['password']);
          unset($data['password']);
        }

        $data['timezone_original'] = $data['timezone'];
        if (in_array($data['timezone'], array('0', '+0', '-0', '0000', '-0000'))) {
          $data['timezone'] = '+0000';
        }
        if (!empty($data['timezone']) && preg_match('/(\+|-)([0][0-9]|[1][01])(00|30|45)/', $data['timezone'], $parts)) {
          $seconds = $parts[2] * 3600 + $parts[3] * 60;
          $sign = $parts[1] == '+' ? '' : '-';
          $data['timezone'] = $sign . $seconds;
        }
      }
      else if ($id == 'role') {
        $data['name'] = $data['value'];
        unset($data['value']);

        if (empty($data['id'])) {
          $rid = db_result(db_query("SELECT rid FROM {role} WHERE name = '%s'", $data['name']));
          if ($rid) {
            $data['id'] = $rid;
          }
        }
      }
      else if ($id == 'permissions') {
        if ($data['role'] && !$data['rid']) {
          $data['rid'] = $data['role'];
        }
        unset($data['role']);
      }
    break;

    // Pre validate actions
    case 'pre-validate':
      if ($id == 'user') {
        // make sure uid is valid
        if (!empty($data['uid']) && !db_result(db_query("SELECT COUNT(*) FROM {users} WHERE uid = %d", $data['uid']))) {
          return t('Invalid user ID supplied: %uid', array('%uid' => $data['uid']));
        }

        // Cannot create/edit/delete anon user
        global $user;

        if ($data['uid'] === 0) {
          return t('Cannot modify anonymous user account.');
        }
        else if ($data['uid'] == 1 && $data['delete']) {
          return t('Cannot delete super-user account. Please modify pattern and try again.');
        }
        else if ($data['uid'] == $user->uid && $data['delete']) {
          return t('You cannot delete the current users account. Please login with a different account and try again.');
        }

        if (empty($data['uid']) && (empty($data['name']) || empty($data['mail']))) {
          return t("<br>1. you didn't supply enough data for creation of the new account (both name and mail are required)<br>or<br>2. account you are trying to update doesn't exist (invalid uid or name).");
        }

        if (!empty($data['timezone'])) {
          $zones = _system_zonelist();
          if (!array_key_exists($data['timezone'], $zones)) {
            return t('Invalid timezone: %zone. Use the following format: +0100 or -0930', array('%zone' => $data['timezone_original']));
          }
        }
        unset($data['timezone_original']);
      }
      else if ($id == 'role') {
        if (DRUPAL_ANONYMOUS_RID == $data['id'] || DRUPAL_AUTHENTICATED_RID == $data['id']) {
          return t('The authenticated or anonymous roles cannot be updated or deleted.');
        }
        if (!empty($data['id']) && !db_result(db_query("SELECT COUNT(*) FROM {role} WHERE rid = %d", $data['id']))) {
          return t('Invalid role id: %rid.', array('%rid' => $data['id']));
        }
      }
      else if ($id == 'permissions') {
        if (!$data['rid']) {
          return t('You must specify a role to apply permissions to.');
        }
      }
    break;

    // Return the form_id('s) for each action
    case 'form_id':
      if ($id == 'user') {
        // Check if the user needs updating
        if ($data['uid']) {
          return $data['delete'] ? 'user_confirm_delete' : 'user_profile_form';
        }
        else {
          return 'user_register';
        }
      }
      else if ($id == 'role') {
        // role names should always be unique
        if (!$data['delete'] && $data['id'] && $data['name'] && db_result(db_query("SELECT COUNT(*) FROM {role} WHERE name = '%s' AND rid != %d", $data['name'], $data['id']))) {
          return;
        }
        return 'user_admin_role';
      }
      else if ($id == 'permissions') {
        return 'user_admin_perm';
      }
    break;

    // Prepare for valid processing of this type of component
    case 'build':
      if ($id == 'user_register') {
        if ($data['uid']) {
          unset($data['uid']);
        }
      }
      else if ($id == 'user_profile_form') {
        module_load_include('inc', 'user', 'user.pages');

        // make sure that required fields are not left empty
        if (empty($data['mail'])) {
          $data['mail'] = db_result(db_query("SELECT mail FROM {users} WHERE uid = %d", $data['uid']));
        }
        if (empty($data['name'])) {
          $data['name'] = db_result(db_query("SELECT name FROM {users} WHERE uid = %d", $data['uid']));
        }

        static $old_q;
        $old_q = $_GET['q'];
        $_GET['q'] = 'user/'. $data['uid'] .'/edit';
      }
      else if ($id == 'user_confirm_delete') {
        module_load_include('inc', 'user', 'user.pages');
        $data['confirm'] = 1;
      }
      else if ($id == 'user_admin_role') {
        module_load_include('inc', 'user', 'user.admin');
        static $old_q;
        $old_q = $_GET['q'];
        if ($data['id']) {
          $_GET['q'] = 'admin/user/roles/edit/'. $data['id'];
        }
        else {
          $_GET['q'] = '';
        }

        if ($data['delete']) {
          $data['op'] = t('Delete role');
        }
        else if ($data['id']) {
          $data['op'] = t('Save role');
        }
        else {
          $data['op'] = t('Add role');
        }
        unset($data['id']);
      }
      else if ($id == 'user_admin_perm') {

        // Get list of permissions to ensure only available permissions are saved
        $permissions = array();
        foreach (module_list(FALSE, FALSE, TRUE) as $module) {
          if ($p = module_invoke($module, 'perm')) {
            $permissions = array_merge($permissions, $p);
          }
        }

        if (is_string($data['value'])) {
          $p = explode(',', $data['value']);
          unset($data['value']);

          foreach($p as $key => $value) {
            $val = trim(trim($value), '\'"');
            if (in_array($val, $permissions)) {
              $perms[$val] = $data['delete'] ? '0' : $val;
            }
          }
          $data[$data['rid']] = $perms;
        }
        else {
          for($i=0;$item=$data[$i];$i++) {
            if (in_array($item, $permissions)) {
              $perms[$item] = ($data['delete'] ? 0 : $item);
            }
            unset($data[$i]);
          }
          $data[$data['rid']] = $perms;
        }


        module_load_include('inc', 'user', 'user.admin');
        if (is_numeric($data['rid'])) {
          $rid = $data['rid'];
        }
        else if (is_string($data['rid'])) {
          $rid = db_result(db_query('SELECT rid FROM {role} WHERE name = "%s"', $data['rid']));
          $role = $data['rid'];
          $data[$rid] = $data[$role];
          $data['rid'] = $rid;
          unset($data[$role]);
        }
        // make sure we don't overwrite all the permissions previously set for the role while assigning the new ones
        if (!$data['overwrite']) {
          $p = db_result(db_query("SELECT perm FROM {permission} WHERE rid = %d", $data['rid']));
          $p = explode(',', $p);
          $perms = array();
          if (!empty($p)) {
            $p = array_map('trim', $p);
            $perms = array_combine($p, $p);
          }
          if (!empty($perms)) {
            $data[$data['rid']] = array_merge($perms, (array)$data[$data['rid']]);
          }
        }
      }
      return $data;
    break;

    // Validate the values for an action before running the pattern
    case 'validate':
      if ($id == 'user_admin_perm') {
        if (is_numeric($data['rid'])) {
          $rid = $data['rid'];
        }
        else if (is_string($data['rid'])) {
          $rid = db_result(db_query('SELECT rid FROM {role} WHERE name = "%s"', $data['rid']));
        }

        if (!db_result(db_query("SELECT COUNT(*) FROM {role} WHERE rid = %d", $rid))) {
          return t('Invalid role %role to set permissions for.', array('%role' => $data['role'] ? $data['role'] : $data['rid']));
        }
      }
    break;

    // Build a patterns actions and parameters
    case 'params':
      if ($id == 'user_confirm_delete' || $id == 'user_profile_form') {
        return array(user_load($data['uid']));
      }
      else if ($id == 'user_admin_perm') {
        return $data['rid'];
      }
    break;

    // Cleanup any global settings or check created data
    case 'cleanup':
      if ($id == 'user_admin_role') {
        static $old_q;
        if ($old_q) {
          $_GET['q'] = $old_q;
          unset($old_q);
        }
      }
      else if ($id == 'user_profile_form') {
        static $old_q;
        $_GET['q'] = $old_q;
      }
    break;

    // Return the primary ID if possible from this action
    case 'identifier':
      switch($id) {
      	case 'user_admin_role':
          return db_result(db_query('SELECT rid FROM {role} WHERE name = "%s"', $data['name']));
        break;
      }
    break;
  }
}
