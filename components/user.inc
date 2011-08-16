<?php


/*
* Implements hook_patterns() for the user module.
* Copied here from patterns/components/user.inc
*/

   function user_patterns($op, $id = NULL, &$data = NULL) {
     
//      $myFile = drupal_realpath('public://') . "/patterns/batch.log";
//      $fh = fopen($myFile, 'a+') or die("can't open file");
//      fwrite($fh, "-----\n");
//      fwrite($fh, date('l jS \of F Y h:i:s A')."\n");
//      $stringData = "op: " . $op ."\n";
//      fwrite($fh, $stringData);
//      $stringData = "id: ". $id ."\n";
//      fwrite($fh, $stringData);
//      $stringData .= "data: " . print_r($data,true);
//      fwrite($fh, $stringData);
//      fwrite($fh, "-----\n");
//      fclose($fh);
     
     
     
  switch ($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('user', 'role', 'permissions');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'user_register_form' => t('User: Add user with the Register Form'),
        'user_profile_form' => t('User: Edit user'),
        'user_cancel_confirm_form' => t('User: Delete user'),
        'user_admin_roles' => t('User: Add role'),
        'user_admin_role' => t('User: Edit role'),
      	'user_admin_role_delete_confirm'=>t('User: Delete role'),
        'user_admin_permissions' => t('User: Configure permissions')
      );
    break;

    // Return a summary of an action
    case 'summary':
      if ($data['uid'] && !$data['name']) {
        // Ste: was
        //$data['name'] = db_result(db_query('SELECT name FROM {users} WHERE uid = "%d"', $data['name']));
          $data['name'] = db_query('SELECT name FROM {users} WHERE uid = :name', array('name' => $data['name']))->fetchField();
      
      }
      $variables = array('%user' => $data['name'], '%uid' => $data['uid'], '%role' => $data['name']);
      switch ($id) {
        // Ste: was
        // case 'user_register':
        case 'user_save':
          return t('Create user %user', $variables);
        break;
        case 'user_profile_form':
          return t('Edit user account %user', $variables);
        break;
        case 'user_cancel_confirm_form':
          return t('Delete user account %user', $variables);
        break;
        case 'user_admin_roles':
          return t('Create user role %role', $variables);
        break;
        case 'user_admin_role':
          return t('Edit user role %role', $variables);
        break;
        case 'user_admin_role_delete_confirm':
          return t('Delete user role %role', $variables);
        break;
        case 'user_admin_permissions':
          return t('Add permissions for %role', $variables);
        break;
      }
    break;

    // Prepare data for processing
    case 'prepare':
      if ($id == 'user') {
        
        // Check with isset first, otherwise we get a notice
        if (!isset($data['name'])) {
          if (!isset($data['uid'])) {
            return t('Cannot create/modify user. Missing name and uid.');
          }
          // Retrieve the username name from the db
          $data['name'] = db_query('SELECT name FROM {users} WHERE uid = :uid', array('uid ' => $data['uid']))->fetchField();
        }
        else {  
          if (!isset($data['uid'])) {
            $data['uid'] = db_query('SELECT uid FROM {users} WHERE name = :name', array('name' => $data['name']))->fetchField();                           
          }
        }
        
        // @TODO: Check if we are overriding the password
        if (isset($data['password'])) {
          $data['pass'] = array('pass1' => $data['password'], 'pass2' => $data['password']);
          
          // Ste: was
          //unset($data['password']);
        }
        
        
        if (isset($data['timezone'])) {
//          if (in_array($data['timezone'], array('0', '+0', '-0', '0000', '-0000'))) {
//            $data['timezone'] = '+0000';
//          }
//         if (!empty($data['timezone']) && preg_match('/(\+|-)([0][0-9]|[1][01])(00|30|45)/', $data['timezone'], $parts)) {
//          $seconds = $parts[2] * 3600 + $parts[3] * 60;
//          $sign = $parts[1] == '+' ? '' : '-';
//          $data['timezone'] = $sign . $seconds;
//          }

          // Something
          
        }
        
        else {
          // Default Europe/Berlin
          $data['timezone'] = 'Europe/Berlin';
        }  
          
       
      }
      elseif ($id == 'role') {
      	if(isset($data['value'])){
        	$data['name'] = $data['value'];
        	unset($data['value']);
      	}

        if (empty($data['rid'])) {
          $rid = db_query("SELECT rid FROM {role} WHERE name = :name", array('name' => $data['name']))->fetchField();
          if ($rid) {
            $data['rid'] = $rid;
          }
        }
      }
      elseif ($id == 'permissions') {
        if (isset($data['role']) && !$data['rid']) {
          $data['rid'] = $data['role'];
        }
        unset($data['role']);
      }
    break;

    // Pre validate actions
    // Pre-Processing
    case 'pre-validate':
      if ($id == 'user') {
        // make sure uid is valid
        // Ste: was
        //if (!empty($data['uid']) && !db_result(db_query("SELECT COUNT(*) FROM {users} WHERE uid = %d", $data['uid']))) {
        if (!empty($data['uid']) && !db_query("SELECT COUNT(*) FROM {users} WHERE uid = :uid", array('uid' => $data['uid']))->fetchField()) {
          return t('Invalid user ID supplied: %uid', array('%uid' => $data['uid']));
        }

        // Cannot create/edit/delete anon user
        global $user;

        if ($data['uid'] === 0) {
          return t('Cannot modify anonymous user account.');
        }
        elseif ($data['uid'] == 1 && $data['delete']) {
          return t('Cannot delete super-user account. Please modify pattern and try again.');
        }
        elseif ($data['uid'] == $user->uid && $data['delete']) {
          return t('You cannot delete the current users account. Please login with a different account and try again.');
        }elseif($data['uid']&& isset($data['create'])){
              return t('The user %name already exist.',array('%name'=>$data['name']));
        }

        if (empty($data['uid']) && (empty($data['name']) || empty($data['mail']))) {
          return t("<br/>1. you didn't supply enough data for creation of the new account (both name and mail are required)<br>or<br>2. account you are trying to update doesn't exist (invalid uid or name).");
        }

        if (!empty($data['timezone'])) {
          
//          echo $data['timezone'];
//          exit;
          
          // Ste: was
          //$zones = _system_zonelist();
          $zones = system_time_zones();
          if (!array_key_exists($data['timezone'], $zones)) {
            return t('Invalid timezone: %zone. Use the following format: America/New_York or Europe/Berlin', array('%zone' => $data['timezone']));
          }
        }
        //unset($data['timezone_original']);
      }
      
      elseif ($id == 'role') {
        if (isset($data['rid'])&&(DRUPAL_ANONYMOUS_RID == $data['rid'] || DRUPAL_AUTHENTICATED_RID == $data['rid'])) {
          return t('The authenticated or anonymous roles cannot be updated or deleted.');
        }
        // Ste: was
        // if (!empty($data['id']) && !db_result(db_query("SELECT COUNT(*) FROM {role} WHERE rid = %d", $data['id']))) {
        if (!empty($data['rid']) && !db_query("SELECT COUNT(*) FROM {role} WHERE rid = :id", array('id' => $data['rid']))->fetchField()) {
          return t('Invalid role id: %rid.', array('%rid' => $data['rid']));
        }
        if(isset($data['create'])&&!empty($data['rid'])){
        	return t('You cannot create a role with fixed rid id');
        }
      }
      elseif ($id == 'permissions') {
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
        	if($data['delete'])
            return 'user_cancel_confirm_form';
          elseif($data['edit'])
            return 'user_profile_form';
          else
            return;
        }
        else {
          // Ste: was
          //return 'user_register';
          if (!$data['delete']) {
            return 'user_register_form';
          }
        }
      }
      elseif ($id == 'role') {
        // role names should always be unique
       if($data['rid'] && $data['name'] && isset($data['edit'])){
        	return 'user_admin_role';
        }
        elseif ($data['delete']) {
          return 'user_admin_role_delete_confirm';
        }
        elseif($data['create']) {
          return 'user_admin_roles';
        }else{
        	return;
        }
      }
      elseif ($id == 'permissions') {
        return 'user_admin_permissions';
      }
    break;

    // Prepare for valid processing of this type of component
    case 'build':
      
      // Ste: was 'user_register'
      if ($id == 'user_register_form') {
        
        if (isset($data['uid'])) {
          unset($data['uid']);
        }

        // Ste: added
//        if (!isset($data['user'])) {
//          $data['user'] = (object) $data;
//        }
        
        // USER_REGISTER_FORM_SUBMIT riceve:

        /*
        
        $form_state Array [20]  
          [0...19]  
            build_info  Array [2] 
            temporary Array [0] 
            programmed  false 
            cache false 
            field Array [4] 
            rebuild false 
            rebuild_info  Array [0] 
            redirect  <Uninitialized> 
            submitted true  
            executed  false 
            method  post  
            groups  Array [3] 
            buttons Array [1] 
            input Array [13]  
            values  Array [21]  
            complete form Array [37]  
            process_input true  
            has_file_element  true  
            triggering_element  Array [19]  
            clicked_button  Array [19]  
        
        $form Array [37]  
  [0...30]  
    #user stdClass  
    #user_category  register  
    #attached Array [1] 
    #attributes Array [2] 
    #validate Array [3] 
    account Array [17]  
    signature_settings  Array [25]  
    picture Array [28]  
    #parents  Array [0] 
    profyle_  Array [18]  
    field_science_discipline  Array [17]  
    profyle_this_is_new_field Array [18]  
    field_sterel  Array [17]  
    #pre_render Array [1] 
    #entity_type  user  
    #bundle user  
    actions Array [16]  
    #submit Array [1] 
    #form_id  user_register_form  
    #type form  
    #build_id form-vLCiTonkQ81dWgjw3MAm9pegriqHj_0vKsOgsuWJNzA  
    form_build_id Array [19]  
    #token  user_register_form  
    form_token  Array [19]  
    form_id Array [19]  
    #id user-register-form  
    #method post  
    #action /admin/people/create?render=overlay&render=overlay&render=overlay 
    #theme_wrappers Array [1] 
    #tree false 
    #theme  Array [1] 
  [31...36] 
    #processed  false 
    #defaults_loaded  true  
    #required false 
    #title_display  before  
    #array_parents  Array [0] 
    #validated  true  
   
            */
        
        // Not here...
        //if (!isset($data['build_info'])) {
        //  $data['build_info'] = array('args' => array(), 'files' => array('modules/user/user.admin.inc'));
        //}
//        $form_state Array [13]  
//      [0...12]  
//    build_info  Array [2] 
//      [0...1] 
//      args  Array [0] 
//      files Array [1] 
//      [0...0] 
//        menu  modules/user/user.admin.inc 
        
//    rebuild false 
//    rebuild_info  Array [0] 
//    redirect  <Uninitialized> 
//    temporary Array [0] 
//    submitted false 
//    executed  false 
//    programmed  false 
//    cache false 
//    method  post  
//    groups  Array [0] 
//    buttons Array [0] 
//    input Array [0] 
        
        
        //user_account_form
        
        
//        $form_state Array [14]  
//  [0...13]  
//    build_info  Array [2] 
//    rebuild false 
//    rebuild_info  Array [0] 
//    redirect  admin/people/create 
//    temporary Array [0] 
//    submitted false 
//    executed  false 
//    programmed  false 
//    cache false 
//    method  post  
//    groups  Array [0] 
//    buttons Array [0] 
//    input Array [0] 
//    field Array [5] 
         
        
      }
      
      
      elseif ($id == 'user_profile_form') {
        module_load_include('inc', 'user', 'user.pages');

        // make sure that required fields are not left empty
        if (empty($data['mail'])) {
          $data['mail'] = db_query("SELECT mail FROM {users} WHERE uid = :uid", array('uid' => $data['uid']))->fetchField();
        }
        if (empty($data['name'])) {
          $data['name'] = db_query("SELECT name FROM {users} WHERE uid = :uid", array('uid' => $data['uid']))->fetchField();
        }

        static $old_q;
        $old_q = $_GET['q'];
        $_GET['q'] = 'user/' . $data['uid'] . '/edit';
      }
      elseif ($id == 'user_cancel_confirm_form') {
        module_load_include('inc', 'user', 'user.pages');        
        $data['confirm'] = 1;
        switch ($data['delete']) {
          case '1' :
            $data['user_cancel_method'] = 'user_cancel_block';
            break;
          case '2' :
            $data['user_cancel_method'] = 'user_cancel_block_unpublish';
            break;
          case '3' :
            $data['user_cancel_method'] = 'user_cancel_reassign';
            break;
          case '4' :
            $data['user_cancel_method'] = 'user_cancel_delete';
            break;
        }
      }
      elseif ($id == 'user_admin_roles' || $id == 'user_admin_role_delete_confirm'||$id == 'user_admin_role') {
        module_load_include('inc', 'user', 'user.admin');
        static $old_q;
        $old_q = $_GET['q'];
        if ($data['rid']) {
        	if($data['delete']){
        		$data['confirm'] = 1;
        		$_GET['q'] = 'admin/people/permissions/roles/delete/' . $data['rid'];
        	}else{
         		 $_GET['q'] = 'admin/people/permissions/roles/edit/' . $data['rid'];
        	}
        }
        else {
          $_GET['q'] = '';
        }

        if ($data['delete']) {
          $data['op'] = t('Delete');
        }
        elseif ($data['rid']) {
          $data['op'] = t('Save role');
        }
        else {
          $data['op'] = t('Add role');
        }
       // unset($data['rid']);
      }
      elseif ($id == 'user_admin_permissions') {

        // Get list of permissions to ensure only available permissions are saved
        $permissions = array();
        foreach (module_list(FALSE, FALSE, TRUE) as $module) {
          if ($p = module_invoke($module, 'permission')) {
            $permissions = array_merge($permissions, array_keys($p));
          }
        }
        
      	if(!empty($data['overwrite'])) {
            	foreach($permissions as $p){
            		$perms[$p]=NULL;
            	}
         }
         
        if (is_string($data['value'])) {
          $p = explode(',', $data['value']);
          unset($data['value']);
          foreach ($p as $key => $value) {
            $val = trim(trim($value), '\'"');
            if (in_array($val, $permissions)) {
            	if (!empty($data['delete'])) {
            		$perms[$val]=NULL;
            	}else{
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
            		$perms[$item]=0;
            	}else{
              		$perms[$item] = $item;
            	}
            }
            unset($data[$i]);
          }
          $data[$data['rid']] = $perms;
        }

        module_load_include('inc', 'user', 'user.admin');
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
      
      return $data;
    break;

    // Validate the values for an action before running the pattern
    case 'validate':
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
    break;

    // Build a patterns actions and parameters
    case 'params':
      if ($id == 'user_cancel_confirm_form') {
        return array(user_load($data['uid']));
      }elseif($id == 'user_admin_role_delete_confirm'||$id='user_admin_role'){
      	return (object)$data;
      }
      elseif ($id == 'user_admin_permissions') {
        return $data['rid'];
      }
    break;

    // Cleanup any global settings or check created data
    case 'cleanup':
      if ($id == 'user_admin_roles' || $id='user_admin_role'||$id == 'user_cancel_confirm_form') {
        static $old_q;
        if ($old_q) {
          $_GET['q'] = $old_q;
          unset($old_q);
        }
      }
      elseif ($id == 'user_profile_form') {
        static $old_q;
        $_GET['q'] = $old_q;
      }
    break;

    // Return the primary ID if possible from this action
    case 'identifier':
      switch ($id) {
        case 'user_admin_role':
          return db_query('SELECT rid FROM {role} WHERE name = :name', array('name' => $data['name']))->fetchField();
        break;
      }
    break;
  }
}


// DR6 FUNCTION

//function user_patterns($op, $id = null, &$data = null) {
//  switch ($op) {
//    // Return the valid tags that this component can prepare and process
//    case 'tags':
//      return array('user', 'role', 'permissions');
//    break;
//
//    // Return a list of forms/actions this component can handle
//    case 'actions':
//      return array(
//        'user_register' => t('User: Add user'),
//        'user_profile_form' => t('User: Edit user'),
//        'user_confirm_delete' => t('User: Delete user'),
//        'user_admin_role' => t('User: Add, edit or remove role'),
//        'user_admin_perm' => t('User: Configure permissions')
//      );
//    break;
//
//    // Return a summary of an action
//    case 'summary':
//      if ($data['uid'] && !$data['name']) {
//        $data['name'] = db_result(db_query('SELECT name FROM {users} WHERE uid = "%d"', $data['name']));
//      }
//      $variables = array('%user' => $data['name'], '%uid' => $data['uid'], '%role' => $data['name']);
//      switch ($id) {
//        case 'user_register':
//          return t('Create user %user', $variables);
//        break;
//        case 'user_profile_form':
//          return t('Edit user account %user', $variables);
//        break;
//        case 'user_confirm_delete':
//          return t('Delete user account %user', $variables);
//        break;
//        case 'user_admin_role':
//          return t('Create user role %role', $variables);
//        break;
//        case 'user_admin_perm':
//          return t('Add permissions for %role', $variables);
//        break;
//      }
//    break;
//
//    // Prepare data for processing
//    case 'prepare':
//      if ($id == 'user') {
//        if ($data['name'] && !$data['uid']) {
//          $data['uid'] = db_result(db_query('SELECT uid FROM {users} WHERE name = "%s"', $data['name']));
//        }
//
//        if ($data['password']) {
//          $data['pass'] = array('pass1' => $data['password'], 'pass2' => $data['password']);
//          unset($data['password']);
//        }
//
//        $data['timezone_original'] = $data['timezone'];
//        if (in_array($data['timezone'], array('0', '+0', '-0', '0000', '-0000'))) {
//          $data['timezone'] = '+0000';
//        }
//        if (!empty($data['timezone']) && preg_match('/(\+|-)([0][0-9]|[1][01])(00|30|45)/', $data['timezone'], $parts)) {
//          $seconds = $parts[2] * 3600 + $parts[3] * 60;
//          $sign = $parts[1] == '+' ? '' : '-';
//          $data['timezone'] = $sign . $seconds;
//        }
//      }
//      elseif ($id == 'role') {
//        $data['name'] = $data['value'];
//        unset($data['value']);
//
//        if (empty($data['id'])) {
//          $rid = db_result(db_query("SELECT rid FROM {role} WHERE name = '%s'", $data['name']));
//          if ($rid) {
//            $data['id'] = $rid;
//          }
//        }
//      }
//      elseif ($id == 'permissions') {
//        if ($data['role'] && !$data['rid']) {
//          $data['rid'] = $data['role'];
//        }
//        unset($data['role']);
//      }
//    break;
//
//    // Pre validate actions
//    case 'pre-validate':
//      if ($id == 'user') {
//        // make sure uid is valid
//        if (!empty($data['uid']) && !db_result(db_query("SELECT COUNT(*) FROM {users} WHERE uid = %d", $data['uid']))) {
//          return t('Invalid user ID supplied: %uid', array('%uid' => $data['uid']));
//        }
//
//        // Cannot create/edit/delete anon user
//        global $user;
//
//        if ($data['uid'] === 0) {
//          return t('Cannot modify anonymous user account.');
//        }
//        elseif ($data['uid'] == 1 && $data['delete']) {
//          return t('Cannot delete super-user account. Please modify pattern and try again.');
//        }
//        elseif ($data['uid'] == $user->uid && $data['delete']) {
//          return t('You cannot delete the current users account. Please login with a different account and try again.');
//        }
//
//        if (empty($data['uid']) && (empty($data['name']) || empty($data['mail']))) {
//          return t("<br/>1. you didn't supply enough data for creation of the new account (both name and mail are required)<br>or<br>2. account you are trying to update doesn't exist (invalid uid or name).");
//        }
//
//        if (!empty($data['timezone'])) {
//          $zones = _system_zonelist();
//          if (!array_key_exists($data['timezone'], $zones)) {
//            return t('Invalid timezone: %zone. Use the following format: +0100 or -0930', array('%zone' => $data['timezone_original']));
//          }
//        }
//        unset($data['timezone_original']);
//      }
//      elseif ($id == 'role') {
//        if (DRUPAL_ANONYMOUS_RID == $data['id'] || DRUPAL_AUTHENTICATED_RID == $data['id']) {
//          return t('The authenticated or anonymous roles cannot be updated or deleted.');
//        }
//        if (!empty($data['id']) && !db_result(db_query("SELECT COUNT(*) FROM {role} WHERE rid = %d", $data['id']))) {
//          return t('Invalid role id: %rid.', array('%rid' => $data['id']));
//        }
//      }
//      elseif ($id == 'permissions') {
//        if (!$data['rid']) {
//          return t('You must specify a role to apply permissions to.');
//        }
//      }
//    break;
//
//    // Return the form_id('s) for each action
//    case 'form_id':
//      if ($id == 'user') {
//        // Check if the user needs updating
//        if ($data['uid']) {
//          return $data['delete'] ? 'user_confirm_delete' : 'user_profile_form';
//        }
//        else {
//          return 'user_register';
//        }
//      }
//      elseif ($id == 'role') {
//        // role names should always be unique
//        if (!$data['delete'] && $data['id'] && $data['name'] && db_result(db_query("SELECT COUNT(*) FROM {role} WHERE name = '%s' AND rid <> %d", $data['name'], $data['id']))) {
//          return;
//        }
//        elseif ($data['delete']) {
//          return 'user_admin_role';
//        }
//        else {
//          return 'user_admin_new_role';
//        }
//      }
//      elseif ($id == 'permissions') {
//        return 'user_admin_perm';
//      }
//    break;
//
//    // Prepare for valid processing of this type of component
//    case 'build':
//      if ($id == 'user_register') {
//        if ($data['uid']) {
//          unset($data['uid']);
//        }
//      }
//      elseif ($id == 'user_profile_form') {
//        module_load_include('inc', 'user', 'user.pages');
//
//        // make sure that required fields are not left empty
//        if (empty($data['mail'])) {
//          $data['mail'] = db_result(db_query("SELECT mail FROM {users} WHERE uid = %d", $data['uid']));
//        }
//        if (empty($data['name'])) {
//          $data['name'] = db_result(db_query("SELECT name FROM {users} WHERE uid = %d", $data['uid']));
//        }
//
//        static $old_q;
//        $old_q = $_GET['q'];
//        $_GET['q'] = 'user/'. $data['uid'] .'/edit';
//      }
//      elseif ($id == 'user_confirm_delete') {
//        module_load_include('inc', 'user', 'user.pages');
//        $data['confirm'] = 1;
//      }
//      elseif ($id == 'user_admin_role' || $id == 'user_admin_new_role') {
//        module_load_include('inc', 'user', 'user.admin');
//        static $old_q;
//        $old_q = $_GET['q'];
//        if ($data['id']) {
//          $_GET['q'] = 'admin/user/roles/edit/'. $data['id'];
//        }
//        else {
//          $_GET['q'] = '';
//        }
//
//        if ($data['delete']) {
//          $data['op'] = t('Delete role');
//        }
//        elseif ($data['id']) {
//          $data['op'] = t('Save role');
//        }
//        else {
//          $data['op'] = t('Add role');
//        }
//        unset($data['id']);
//      }
//      elseif ($id == 'user_admin_perm') {
//
//        // Get list of permissions to ensure only available permissions are saved
//        $permissions = array();
//        foreach (module_list(FALSE, FALSE, TRUE) as $module) {
//          if ($p = module_invoke($module, 'perm')) {
//            $permissions = array_merge($permissions, $p);
//          }
//        }
//
//        if (is_string($data['value'])) {
//          $p = explode(',', $data['value']);
//          unset($data['value']);
//
//          foreach ($p as $key => $value) {
//            $val = trim(trim($value), '\'"');
//            if (in_array($val, $permissions)) {
//              $perms[$val] = $val;
//            }
//          }
//          $data[$data['rid']] = $perms;
//        }
//        else {
//          for($i=0;$item=$data[$i];$i++) {
//            if (in_array($item, $permissions)) {
//              $perms[$item] = $item;
//            }
//            unset($data[$i]);
//          }
//          $data[$data['rid']] = $perms;
//        }
//
//
//        module_load_include('inc', 'user', 'user.admin');
//        if (is_numeric($data['rid'])) {
//          $rid = $data['rid'];
//        }
//        elseif (is_string($data['rid'])) {
//          $rid = db_result(db_query('SELECT rid FROM {role} WHERE name = "%s"', $data['rid']));
//          $role = $data['rid'];
//          $data[$rid] = $data[$role];
//          $data['rid'] = $rid;
//          unset($data[$role]);
//        }
//
//        if (!empty($data['delete'])) {
//          $data[$data['rid']]['delete'] = 1;
//        }
//        if (!empty($data['overwrite'])) {
//          $data[$data['rid']]['overwrite'] = 1;
//        }
//      }
//      return $data;
//    break;
//
//    // Validate the values for an action before running the pattern
//    case 'validate':
//      if ($id == 'user_admin_perm') {
//        if (is_numeric($data['rid'])) {
//          $rid = $data['rid'];
//        }
//        elseif (is_string($data['rid'])) {
//          $rid = db_result(db_query('SELECT rid FROM {role} WHERE name = "%s"', $data['rid']));
//        }
//
//        if (!db_result(db_query("SELECT COUNT(*) FROM {role} WHERE rid = %d", $rid))) {
//          return t('Invalid role %role to set permissions for.', array('%role' => $data['role'] ? $data['role'] : $data['rid']));
//        }
//      }
//    break;
//
//    // Build a patterns actions and parameters
//    case 'params':
//      if ($id == 'user_confirm_delete' || $id == 'user_profile_form') {
//        return array(user_load($data['uid']));
//      }
//      elseif ($id == 'user_admin_perm') {
//        return $data['rid'];
//      }
//    break;
//
//    // Cleanup any global settings or check created data
//    case 'cleanup':
//      if ($id == 'user_admin_role') {
//        static $old_q;
//        if ($old_q) {
//          $_GET['q'] = $old_q;
//          unset($old_q);
//        }
//      }
//      elseif ($id == 'user_profile_form') {
//        static $old_q;
//        $_GET['q'] = $old_q;
//      }
//    break;
//
//    // Return the primary ID if possible from this action
//    case 'identifier':
//      switch ($id) {
//      	case 'user_admin_role':
//          return db_result(db_query('SELECT rid FROM {role} WHERE name = "%s"', $data['name']));
//        break;
//      }
//    break;
//  }
//}
