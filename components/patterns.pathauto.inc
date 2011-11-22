<?php
/*
 * @file
 * Patterns component for pathauto.
 */
function pathauto_patterns() {
  $files = array('sites/all/modules/pathauto/pathauto.admin.inc');

  $actions['pathauto'] = array(
    'descr' => t('Setting pathauto'),
    PATTERNS_MODIFY => array('pathauto_settings_form'),
    PATTERNS_FILES => $files,
  );
  $actions['url_alias'] = array(
    'descr' => t('create/edit/delit url aliases'),
    PATTERNS_CREATE => array('path_admin_form'),
    PATTERNS_MODIFY => array('path_admin_form'),
    PATTERNS_DELETE => array('path_admin_delete_confirm'),
    PATTERNS_FILES => $files,
  );
  $actions['pathauto_patterns'] = array(
    'descr' => t('setting patterns for auto URLs aliases'),
    PATTERNS_MODIFY => array('pathauto_patterns_form', 'pathauto_bulk_update_form'),
    PATTERNS_FILES => $files,
  );

  return $actions;

}

function pathauto_patterns_actions() {
  return array(
    'pathauto_admin_form' => t('Pathauto: Create/edit url aliases'),
    'path_admin_delete_confirm' => t('Pathauto: Delit url aliases'),
    'pathauto_settings_form' => t('Pathauto: Configure pathauto settings'),
    'pathauto_patterns_form' => t('Pathauto: Configure patterns settings for auto URLs aliases'),
    'pathauto_bulk_update_form' => t('Pathauto: Generate URL aliases according to patterns settings'),
  );
}

// Prepare data for processing
function pathauto_patterns_prepare($action, $tag, &$data = NULL) {
  $datacopy = $data;
  if ($tag == "pathauto_patterns" || $tag == "pathauto") {
    foreach ($datacopy as $key => $value) {
      if (is_array($value)) {
        if ( $key != "update") {
          foreach ($value as $i => $v) {
            $new =  'pathauto_' . $i . '_' . $key;
            $data[$new] = $v;
            unset($data[$key][$i]);
          }
          unset($data[$key]);
        }
        elseif ($key == "update") {
          foreach ($value as $i => $v) {
            $new =  $i . '_pathauto_bulk_update_batch_process';
            if ($v == 1) {
              $data[$key][$new] = $new;
            }
            unset($data[$key][$i]);
          }
        }
      }
      else {
        $new = 'pathauto_' . $key;
        $data[$new] = $value;
        unset($data[$key]);
      }

    }
  }
  elseif ($tag == "url_alias") {
    if ($action == PATTERNS_DELETE) {
      if (isset($data['pid'])) {
        $data['path']['pid'] = $data['pid'];
      }
      $data['confirm'] = 1;
    }
  }
  return patterns_results();
}

// Validate the values for an action before running the pattern
function pathauto_patterns_validate($action, $tag, &$data = NULL) {
  $status = PATTERNS_SUCCESS;
  $msg = '';
  if ($tag == 'url_alias' && ( $action == PATTERNS_MODIFY || $action == PATTERNS_DELETE)) {
    if (!isset($data['pid'])) {
      $status = PATTERNS_ERR;
      $msg = t('Attempt to %update a nonexistent url alias.', array('%update' => $action));
    }
    else {
      $exist = db_query("SELECT COUNT(*) FROM {url_alias} WHERE pid = :pid", array('pid' => $data['pid']))->fetchField();
      if (!$exist) {
        $status = PATTERNS_ERR;
        $msg = t('Attempt to %update a nonexistent url alias.', array('%update' => $action));
      }
    }
  }
  elseif ($tag == 'url_alias' && $action == PATTERNS_CREATE) {
    $lan = isset($data['language']) ? $data['language'] : 'und';
    $exist = db_query("SELECT COUNT(*) FROM {url_alias} WHERE alias = :name and language = :lan", array('name' => $data['alias'], 'lan' => $lan))->fetchField();
    if ($exist) {
      $status = PATTERNS_ERR;
      $msg = t('The alias %path is already in use in this language.', array('%path' => $data['alias']));
    }
  }
  return patterns_results($status, $msg);
}

// Prepare for valid processing of this type of component
function pathauto_patterns_build($action, $tag, &$data = NULL, $form_id) {
  if ($data['pathauto_reset']) {
    $result = db_query("SELECT name FROM {variable} WHERE name LIKE 'pathauto_%'");
    while ($var = db_fetch_array($result)) {
      variable_del($var['name']);
    }
    cache_clear_all('variables', 'cache');
    return;
  }

  module_load_include('inc', 'pathauto', 'pathauto.admin');
  module_load_include('inc', 'path', 'path.admin');
  return patterns_results(PATTERNS_SUCCESS,   t('Execution successful'), $data);
}

// Build a patterns actions and parameters
function pathauto_patterns_params($action, $form_id, &$data = NULL, &$a) {
  if ($form_id == "path_admin_form" && $action == PATTERNS_MODIFY) {
    $pid = $data['pid'];
    $path = db_query("SELECT * FROM {url_alias} WHERE pid = :pid", array('pid' => $data['pid']))->fetchAll();
    $result = array((array)$path[0]);
  }
  elseif ($form_id == "path_admin_form") {
    $result = array(array(
        'source' => '',
        'alias' => '',
        'language' => 'und',
        'pid' => NULL,
      ));
  }
  elseif ($form_id == "path_admin_delete_confirm") {
    $result = array($data['path']);
  }
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

function pathauto_patterns_cleanup($action, $tag, &$data = NULL) {
  //unset($_POST['op']);
  return patterns_results();
}

// Return which callback functions to actually use.
function pathauto_patterns_callbacks($action, $tag, &$data = NULL) {
  $desc = pathauto_patterns();
  $result = $desc[$tag][$action];
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}



/*function pathauto_patterns($op, $id = NULL, &$data = NULL) {
 switch ($op) {
 // Return the valid tags that this component can prepare and process
 case 'tags':
 return array('pathauto');
 break;

 // Return a list of forms/actions this component can handle
 case 'actions':
 return array(
 'pathauto_admin_settings' => t('Pathauto: Configure pathauto settings'),
 );
 break;

 // Return a summary of an action
 case 'summary':
 return t('Setup pathauto urls');
 break;

 // Prepare data for processing
 case 'prepare':
 foreach ($data as $key => $value) {
 if (is_array($value)) {
 $mappings = array('default' => 'pattern');
 $skip = array('bulkupdate', 'applytofeeds');
 foreach ($value as $i => $v) {
 if (in_array($i, $skip)) {
 $new = $key . '_' . $i;
 }
 elseif (array_key_exists($i, $mappings)) {
 $new = $key . '_' . $mappings[$i];
 }
 // remove prefix "vid_" required by XML because
 // element names can't begin with a number
 elseif ($key == 'taxonomy' && strpos($i, 'vid_') === 0) {
 $new = $key . '_' . str_replace('vid_', '', $i) . '_pattern';
 }
 else {
 $new = $key . '_' . $i . '_pattern';
 }

 $data[$new] = $v;
 unset($data[$key][$i]);
 }
 unset($data[$key]);
 }
 }

 $mappings = array('update' => 'update_action');
 foreach ($data as $key => $value) {
 if (array_key_exists($key, $mappings)) {
 $new = $mappings[$key];
 }
 else {
 $new = 'pathauto_' . $key;
 }

 if (strpos($key, 'pathauto_') === FALSE && $key != 'reset') {
 $data[$new] = $value;
 unset($data[$key]);
 }
 }
 break;

 // Pre validate actions
 case 'pre-validate':
 break;

 // Return the form_id('s) for each action
 case 'form_id':
 module_load_include('inc', 'pathauto', 'pathauto.admin');
 return 'pathauto_admin_settings';
 break;

 // Prepare for valid processing of this type of component
 case 'build':
 if ($data['reset']) {
 $result = db_query("SELECT name FROM {variable} WHERE name LIKE 'pathauto_%'");
 while ($var = db_fetch_array($result)) {
 variable_del($var['name']);
 }
 cache_clear_all('variables', 'cache');
 return;
 }

 return $data;
 break;

 // Validate the values for an action before running the pattern
 case 'validate':
 break;

 // Build a patterns actions and parameters
 case 'params':
 break;

 // Cleanup any global settings after the action runs
 case 'cleanup':
 unset($_POST['op']);
 break;
 }
 }*/
