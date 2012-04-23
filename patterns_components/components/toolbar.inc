<?php
/*
 * @file
 * Patterns component for toolbar.
 */

/**
 * hook_patterns()
 */
function toolbar_patterns($data) {
  $files = array('modules/user/user.admin.inc');
  $actions['toolbarrole'] = array(
    PATTERNS_INFO => t('Decide who can see the toolbar'),
    PATTERNS_MODIFY => array('user_admin_permissions'),
    PATTERNS_FILES => $files,
  );
  return $actions;
}

function toolbar_patterns_actions($data) {
  return array(
    'user_admin_permissions' => t('Decide who can use toolbar'),
    );
}

/**
 * hook_patterns_prepare()
 */
function toolbar_patterns_prepare($action, $tag, &$data) {

  module_load_include('module', 'user', 'user');
  $data['role_names'] = user_roles();
  $data['role_permissions'] = user_role_permissions($data['role_names']);

  //get all the module's name which inplement the hook_permission function.
  $module_info = system_get_info('module');
  $modules = array();
  foreach (module_implements('permission') as $module) {
    $modules[$module] = $module_info[$module]['name'];
  }
  asort($modules);

  //three foreach,every module inplementing the hook_permission,every permission from module's hook_permission,every role for every permission.
  //$data['status'] get every rid's permission. asort by module's name.
  foreach ($modules as $module => $display_name) {
    if ($permissions = module_invoke($module, 'permission')) {
      foreach ($permissions as $perm => $perm_item) {
        foreach ($data['role_names'] as $rid => $name) {
          // Builds arrays for checked boxes for each role
          if (isset($data['role_permissions'][$rid][$perm])) {
            $data['status'][$rid][] = $perm;
          }
        }
      }
    }
  }

  return patterns_results();
}

/**
 * hook_patterns_validate()
 */
function toolbar_patterns_validate($action, $tag, &$data) {
  $status = PATTERNS_SUCCESS;
  $msg = '';

  //if both addroles and delroles is not given, there is a error.
  if (!(isset($data['addroles'])) && !(isset($data['delroles']))) {
    $status = PATTERNS_ERR;
    $msg = t('there is nothing to do.');
    return patterns_results($status, $msg);
  }

 //if there is a same name in add and delroles, there is a error.
  if (is_array($data['addroles']) && is_array($data['delroles'])) {
    $a = array_intersect($data['addroles'], $data['delroles']);
    if (count($a) != 0) {
      $status = PATTERNS_ERR;
      $msg = t('same name in both addroles and delroles');
      return patterns_results($status, $msg);
    }
  }

  //the process of addroles.
  //role can not be administrator
  //the name should exist in the role table.
  //if the role already have the perm, there is a error.
  if (is_array($data['addroles'])) {
    foreach ($data['addroles'] as $key => $value) {
      $rid = db_select('role', 'r')
              ->fields('r', array('rid'))
              ->condition('r.name', $value)
              ->execute()
              ->fetchField();
      if ($rid == 3) {
        $status = PATTERNS_ERR;
        $msg = t('administrator should keep static.');
        return patterns_results($status, $msg);
      }
      if ($rid == FALSE) {
        $status = PATTERNS_ERR;
        $msg = t('there is a wrong role name');
        return patterns_results($status, $msg);
      }
      if (isset($data['status'][$rid])) {
        if (in_array("access toolbar", $data['status'][$rid])) {
          $status = PATTERNS_ERR;
          $msg = t('the role have the toolbar permission already.');
          return patterns_results($status, $msg);
        }
      }

      //add the "access toolbar" perm to the $data['status']
      $data['status'][$rid][] = "access toolbar";
    }
  }

  //the process of delroles.
  //role can not be administrator
  //the name should not exist in the role table.
  //if the role don't have the perm yet, there is a error.
  if (is_array($data['delroles'])) {
    foreach ($data['delroles'] as $key => $value) {
      $rid = db_select('role', 'r')
              ->fields('r', array('rid'))
              ->condition('r.name', $value)
              ->execute()
              ->fetchField();
      if ($rid == 3) {
        $status = PATTERNS_ERR;
        $msg = t('administrator should keep static.');
        return patterns_results($status, $msg);
      }
      if ($rid == FALSE) {
        $status = PATTERNS_ERR;
        $msg = t('there is a wrong role name');
        return patterns_results($status, $msg);
      }
      //ensure the role have some perms.
      if (isset($data['status'][$rid])) {
        if (!(in_array("access toolbar", $data['status'][$rid]))) {
          $status = PATTERNS_ERR;
          $msg = t('the role do not have the toolbar permission yet.');
          return patterns_results($status, $msg);
        }
        //delete the "access toolbar" from $data['status']
        $data['status'][$rid] = array_diff($data['status'][$rid], array("access toolbar"));
      }
      else {
        $status = PATTERNS_ERR;
        $msg = t('the role does not have any permissions.');
        return patterns_results($status, $msg);
      }
    }
  }

  //the $data[$rid] is the form's element.
  foreach ($data['status'] as $rid => $value) {
    $data[$rid] = $value;
  }

  return patterns_results($status, $msg);
}

/**
 * hook_patterns_callbacks()
 */
function toolbar_patterns_callbacks($action, $tag, &$data) {
  $result = array('user_admin_permissions');
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

/**
 * hook_patterns_build()
 */
function toolbar_patterns_build($action, $form_id, &$data = NULL, &$a) {
  module_load_include('inc', 'user', 'user.admin');
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful(in build function)'), $data);
}


/**
 * hook_patterns_params()
 */
function toolbar_patterns_params($action, $form_id, &$data = NULL, &$a) {
  return patterns_results();
}

/**
 * hook_patterns_cleanup()
 */
function toolbar_patterns_cleanup($action, $tag, &$data) {
  return patterns_results();
}
