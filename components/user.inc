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
        'user_register' => t('Create new user'),
        'user_edit' => t('Edit existing user'),
        'user_confirm_delete' => t('Delete existing user'),
        'user_admin_role' => t('Add/remove user roles'),
        'user_admin_perm' => t('Change user role\'s permissions')
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
        case 'user_edit':
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
        if ($data['delete'] && $data['name'] && !$data['uid']) {
          $data['uid'] = db_result(db_query('SELECT uid FROM {users} WHERE name = "%s"', $data['name']));
        }
      }
      else if ($id == 'role') {
        $data['name'] = $data['value'];
        unset($data['value']);
        
        $rid = db_result(db_query("SELECT rid FROM {role} WHERE name = '%s'", $data['name']));
        if ($rid) {
          $data['id'] = $rid;
        }
      }
      
      if ($data['password']) {
        $data['pass'] = array('pass1' => $data['password'], 'pass2' => $data['password']);
        unset($data['password']);
      }
      
      if ($id == 'permissions') {
        // Get list of permissions to ensure only available permissions are saved
        $permissions = array();
        foreach (module_list(FALSE, FALSE, TRUE) as $module) {
          if ($p = module_invoke($module, 'perm')) {
            $permissions = array_merge($permissions, $p);
          }
        }
        
        if ($data['role'] && !$data['rid']) {
          $data['rid'] = $data['role'];
        }
        unset($data['role']);
        
        if ($data['rid']) {
          if (is_string($data['value'])) {
            $perms = explode(',', $data['value']);
            unset($data['value']);
            
            foreach($perms as $key => $value) {
              if (!in_array($key, $permissions)) {
                continue;
              }
              
              $perms[$key] = trim($value);
            }
            
            $data[$data['rid']] = array_combine(array_values($perms), array_values($perms));
          }
          else {
            for($i=0;$item=$data[$i];$i++) {
              if (in_array($data[$i], $permissions)) {
                $perms[] = $item;
              }
              
              unset($data[$i]);
            }
            
            $data[$data['rid']] = array_combine(array_values($perms), array_values($perms));
          }
        }
      }
    break;
    
    // Pre validate actions
    case 'pre-validate':
      if ($id == 'user') {
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
      }
      else if ($id == 'role') {
        if (DRUPAL_ANONYMOUS_RID == $data['id'] || DRUPAL_AUTHENTICATED_RID == $data['id']) {
          return t('The authenticated or anonymous roles cannot be deleted.');
        }
      }
      else if ($id == 'permissions') {
        if (!$data['rid']) {
          return t('You must specify a role to apply permissions to');
        }
      }
    break;
    
    // Return the form_id('s) for each action
    case 'form_id':
      if ($id == 'user') {
        // Check if the user needs updating
        if ($data['uid'] && db_result(db_query('SELECT uid FROM {users} WHERE uid = "%d"', $data['uid']))) {
          return $data['delete'] ? 'user_confirm_delete' : 'user_edit';
        }
        else if (db_result(db_query('SELECT uid FROM {users} WHERE name = "%s"', $data['name']))) {
          return $data['delete'] ? 'user_confirm_delete' : 'user_edit'; 
        }
        else {
          return 'user_register';
        }
      }
      else if ($id == 'role') {
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
      if ($data['roles']) {
        foreach($data['roles'] as $key => $value) {
          if (is_string($key)) {
            $rid = db_result(db_query('SELECT rid FROM {role} WHERE name = "%s"', $key));
            
            if (is_numeric($rid)) {
              $data['roles'][$rid] = $value;
              unset($data['roles'][$key]);
            }
          }
        }
      }
      
      if ($id == 'user_register') {
        if ($data['uid']) {
          unset($data['uid']);
        }
      }
      else if ($id == 'user_edit') {
        if (!$data['uid']) {
          $data['uid'] = db_result(db_query('SELECT uid FROM {users} WHERE name = "%s"', $data['name']));
        }
        
        static $old_q;
        $old_q = $_GET['q'];
        $_GET['q'] = 'user/'. $data['uid'] .'/edit';
      }
      else if ($id == 'user_confirm_delete') {
        $data['confirm'] = 1;
      }
      else if ($id == 'user_admin_role') {
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
        if (is_string($data['rid'])) {
          $rid = db_result(db_query('SELECT rid FROM {role} WHERE name = "%s"', $data['rid']));
          $role = $data['rid'];
        }
        else {
          $rid = $data['rid'];
        }
        unset($data['rid']);
        
        $data[$rid] = $data[$role];
        unset($data[$role]);
      }
      
      return $data;
    break;
    
    // Validate the values for an action before running the pattern
    case 'validate':
      if ($id == 'user_register' && $data['uid']) {
        if (db_result(db_query('SELECT uid FROM {users} WHERE uid = "%d"', $data['uid']))) {
          drupal_set_message(t('Warning: User %name not created with the desired id of %uid', array('%name' => $data['name'], '%uid' => $data['uid'])));
        }
      }
      
      if ($data['roles']) {
        foreach($data['roles'] as $key => $value) {
          if (is_string($key)) {
            $rid = db_result(db_query('SELECT rid FROM {role} WHERE name = "%s"', $key));
            
            if (!$rid) {
              return t('Invalid role %role', array('%role' => $key));
            }
          }
        }
      }
      
      if ($id == 'user_admin_perm') {
        if (is_string($data['rid'])) {
          $rid = db_result(db_query('SELECT rid FROM {role} WHERE name = "%s"', $data['rid']));
        }
        else if (is_numeric($data['rid'])) {
          $rid = $data['rid'];
        }
        
        if (!$rid) {
          return t('Invalid role %role to set permissions for', array('%role' => $data['role'] ? $data['role'] : $data['rid']));
        }
      }
    break;
    
    // Build a patterns actions and parameters
    case 'params':
      if ($id == 'user_confirm_delete') {
        return array($data['name'], $data['uid']);
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
      else if ($id == 'user_edit') {
        static $old_q;        
        $_GET['q'] = $old_q;
      }
    break;
    
    // Reverse actions when disabling a pattern
    case 'reverse':
      if ($id == 'user') {
        if ($data['delete']) {
          unset($data['delete']);
        }
        else {
          $data['delete'] = true;
        }
      }
      else if ($id == 'role') {
        if ($data['delete']) {
          unset($data['delete']);
        }
        else {
          $data['delete'] = true;
        }
      }
      
      return $data;
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
