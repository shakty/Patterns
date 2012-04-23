<?php
/*
 * @file
 * Patterns component for Content Types and Nodes.
 */
function node_patterns($data) {

  $files_ct = array('modules/node/content_types.inc');
  $files_n = array('modules/node/node.pages.inc');

  // Determine necessary forms for action 'node' based on the data.
  $forms = array();
  if ($data) {
    if ($data['tag'] == 'node' && isset($data['type'])) {
      $forms[] = $data['type'] . '_node_form';
    }
  }

  $actions['node'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete Nodes'),
    PATTERNS_CREATE => $forms,
    PATTERNS_MODIFY => $forms,
    PATTERNS_DELETE => array('node_delete_confirm'),
    PATTERNS_FILES  => $files_n,
    PATTERNS_EXPORT => array(PATTERNS_EXPORT_ALL => 'node_patterns_export_all_nodes', ),
    //PATTERNS_EXPORT => array(
    //                      PATTERNS_EXPORT_ALL => 'function1',
    //                      'Articles' => 'export_generic_node',
    //                      'This node' => 'export_single_node',
    //                   ),
  );

  $actions['content_type'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete Content Types'),
    PATTERNS_CREATE => array('node_type_form'),
    PATTERNS_MODIFY => array('node_type_form'),
    PATTERNS_DELETE => array('node_type_delete_confirm'),
    PATTERNS_FILES  => $files_ct,
    PATTERNS_EXPORT => array(PATTERNS_EXPORT_ALL => 'node_patterns_export_all_content_types', ),
    //PATTERNS_EXPORT => array(
    //                      PATTERNS_EXPORT_ALL => 'ccctttypes',
    //                      'ctype' => 'ccctttypes1',
    //                      'ctype2' => 'ccctttypes2',
    //                   ),
  );

  $actions['content_type_extract_all'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete Content Types'),
    PATTERNS_MODIFY => array('node_type_form'),
    PATTERNS_FILES  => $files_ct,
  );

  return $actions;

}

function node_patterns_export_all_content_types($args = NULL, &$result = NULL) {
  // Load content types.
  $types = _node_types_build()->types;
  $result = array();
  foreach ($types as $type) {
    $type = (array) $type;
    $data = array('tag' => 'content_type');
    $data = array_merge($data, $type);
    $action = array(PATTERNS_CREATE => $data);
    array_push($result, $action);
  }
  return $result;
}

function node_patterns_export_all_nodes($args = NULL, &$result = NULL) {
  // Load nodes.
  $nodes = node_load_multiple(FALSE);
  $result = array();
  foreach ($nodes as $node) {
    $node = (array) $node;
    $data = array('tag' => 'node');
    $data = array_merge($data, $node);
    $action = array(PATTERNS_CREATE => $data);
    array_push($result, $action);
  }
  return $result;
}


function node_patterns_actions($data) {
  return array(
    'node_delete_confirm' => t('Delete Nodes'),
    'node_type_form' => t('Create or Edit Content Types'),
    'node_type_delete_confirm' => t('Delete Content Types'),
  );
}

// returns arguments needed for the forms when processing automatically
// $loop is set if we want to extract all values
function node_patterns_get_arguments($action, $tag, $form_id, &$loop = FALSE) {
  if ($tag == 'content_type' && $action == PATTERNS_MODIFY) {
      $types = (object) _node_types_build();
      foreach (array_values($types->types) as $type) {
        return array($type);
      }
      return array(); // ???
  }
  elseif ($tag == 'content_type_extract_all') {
    $loop = TRUE;
    if ($form_id == 'node_type_form') {
      $types = (object) _node_types_build();
      $return = array();
      foreach (array_values($types->types) as $type) {
        array_push($return, array($type));
      }
      return $return;
    }
  }
}


function node_patterns_prepare($action, $tag, &$data) {
  if ($tag == 'content_type') {
    if ($action === PATTERNS_CREATE or $action === PATTERNS_MODIFY) {
      if ($data['node_options']) {
        $data['node_options'] = array_values($data['node_options']);
      }
      else {
        $data['node_options'] = array('status', 'promote'); // TODO: proper defaults?
      }
    }
  // TODO: check if the name and type fields are unique when creating
  //       and existing when we are modifying or deleting
  }
  elseif ($tag == 'node') {
    global $user;

    // Use 'nid' instead of 'id'.
    if (empty($data['nid']) && !empty($data['id'])) {
      $data['nid'] = $data['id'];
    }
    unset($data['id']);

    // TODO: are these needed?
    if (empty($data['name']) && !empty($data['author'])) {
      $data['name'] = $data['author'];
    }
    unset($data['author']);

    if (empty($data['uid'])) $data['uid'] = $user->uid;
    if (empty($data['name'])) $data['name'] = $user->name;

    if ($action == PATTERNS_CREATE) {
      unset($data['nid']);
      unset($data['vid']);
      $data['node'] = (object) $data;
      $data['update'] = FALSE;
    }
    elseif ($action == PATTERNS_MODIFY) {
      $data['update'] = TRUE;
    }
    elseif ($action == PATTERNS_DELETE) {
      $data['update'] = FALSE;
    }
  }
  return patterns_results();
}

function node_patterns_validate($action, $tag, &$data) {
  $status = PATTERNS_SUCCESS;
  $msg = '';
  $result = NULL;

  if ($tag == 'node') {
    if ($action !== PATTERNS_CREATE) {
      $data['node'] = node_load($data['nid']);
      if ($data['node'] === FALSE) {
        $status = PATTERNS_ERR;
        $msg = t('Cannot modify or delete a nonexistent Node.');
      }
      else{
        $data['node'] = (object) $data['node'];
        if ($action === PATTERNS_MODIFY && $data['type'] != $data['node']->type) {
          $status = PATTERNS_ERR;
          $msg = t("You can't change content type for already existing node.");
        }
      }
    }

    if (empty($data['type']) && $action === PATTERNS_CREATE) {
      $status = PATTERNS_ERR;
      $msg = t('"type" field is required.');
    }
    elseif ($action === PATTERNS_DELETE && empty($data['nid'])) {
      $status = PATTERNS_ERR;
      $msg = t('"id" field is required.');
    }
  // TODO: validate name field - should be valid/existing username
  }
  elseif ($tag == 'content_type') {
    // TODO: check required fields for content types
  }

  return patterns_results($status, $msg, $result);
}

function node_patterns_callbacks($action, $tag, &$data) {
  if ($tag == 'node') {
      if ($action === PATTERNS_CREATE or $action === PATTERNS_MODIFY) {
        $result = array($data['type'] . '_node_form');
      }
      else {
        $result = array('node_delete_confirm');
      }
  }
  elseif ($tag == 'content_type') {
    if ($action === PATTERNS_CREATE or $action === PATTERNS_MODIFY) {
      $result = array(
        'node_type_form',
      );
    }
    elseif ($action === PATTERNS_DELETE) {
      $result = array(
        'node_type_delete_confirm',
      );
    }
  }
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Prepare for valid processing of this type of component
function node_patterns_build($action, $form_id, &$data = NULL, &$a) {
  $status = PATTERNS_SUCCESS;
  $msg = '';
  $result = NULL;

  //content_clear_type_cache();
  // TODO: instead: cache_clear_all();
  module_load_include('inc', 'content', 'includes/content.admin');
  module_load_include('inc', 'node', 'content_types'); // TODO: Isn't this already loaded?
  if ($form_id == 'node_delete_confirm') {
    $data['confirm'] = 1;
  }
  elseif ($form_id == 'node_type_form') {

    module_load_include('inc', 'node', 'content_types');

    $type = $data['type'];
    $name = $data['name'];
    /*
    $types = node_type_get_names();

    if (!isset($types[$type])) {
      // Set the name of the new content type if missing
      if (!$data['name']) {
        $data['name'] = $type;
      }

      $defaults = _node_type_set_defaults($data);
      $data = array_merge($defaults, $data);
    }
    */

    // TODO: update these
    /*
    if (empty($data['node_options'])) {
      $node_options = array('status', 'promote', 'sticky', 'revision');
      $default_options = variable_get('node_options_' . $type, NULL);

      $options = array();
      if (!empty($default_options)) {
        $options = array_combine($default_options, $default_options);
      }

      foreach ($data as $key => $value) {
        if (in_array($key, $node_options)) {
          if (!empty($value)) {
            $options[$key] = $key;
          }
          else {
            unset($options[$key]);
          }
        unset($data[$key]);
        }
      }
      $data['node_options'] = array_values($options);
      $data['node_options']['overwrite'] = TRUE;
    }
    */


    // TODO
    if (isset($data['has_body']) && !$data['has_body']) {
      $data['body_label'] = '';
    }
  }
  elseif ($form_id == 'node_type_delete_confirm') {
    $data['op'] = t('Delete');
    $data['confirm'] = 1;
  }
  else { // $data['type'] . '_node_form'
    $data['changed'] = time();
    $data['op'] = t('Save');

    $node = clone($data['node']);
//    unset($node->body);
//    unset($node->title);

    // build Content Type fields
    $type = $data['type'];
    $form_id = $type . '_node_form';
    $form_state = array(
      'build_info' => array('args' => array($node),
        'files' => array('menu' => 'modules/node/node.pages.inc')),
// TODO: all these needed?
      'rebuild' => FALSE,
      'rebuild_info' => array(),
      'redirect' => NULL,
      'temporary' => array(),
      'submitted' => FALSE,
      'executed' => FALSE,
      'programmed' => FALSE,
      'cache' => FALSE,
      'method' => 'post',
      'groups' => array(),
      'buttons' => array(),
      'input' => array(),
    );
    require_once('modules/node/node.pages.inc');

    $form = drupal_retrieve_form($form_id, $form_state);
    drupal_prepare_form($form_id, $form, $form_state);

    /*
    $content_type = content_types($type);
    $fields_info = $content_type['fields'];
    if (module_exists('fieldgroup')) {
      $groups = fieldgroup_groups($type, FALSE, TRUE);
    }

    $fields = array();
    $fields_diff = array();
    if (!empty($groups)) {
      foreach($groups as $key => $group) {
        $fields = array_merge($fields, array_intersect_ukey($form[$key], $group['fields'], '_patterns_compare_keys'));
      }
      $fields_diff = array_diff_ukey($fields_info, $fields, '_patterns_compare_keys');
      $fields = array_merge($fields, array_intersect_ukey($form, $fields_diff, '_patterns_compare_keys'));
    }
    else {
      $fields = array_merge($fields, array_intersect_ukey($form, $fields_info, '_patterns_compare_keys'));
    }

     */
  }

  $result = $data;

  return patterns_results($status, $msg, $result);
}

function node_patterns_params($action, $form_id, &$data = NULL, &$a) {
  $status = PATTERNS_SUCCESS;
  $msg = '';
  $result = NULL;

  if ($form_id == 'node_delete_confirm') {
    $result = array((object) $data);
  }
  elseif (($action === PATTERNS_MODIFY and $form_id == 'node_type_form') or
    $form_id == 'node_type_delete_confirm') {
      $types = (object)_node_types_build();
      $type = $types->types[$data['type']];
      $result = array($type);
    }
  else { // other cases, Node create&modify
    $data['node']->language = 'und';
    $result = clone($data['node']);

    //  $result = array(array(
    //    'bundle' => 'article',
    //    'entity_type' => 'node',
    ////    'field_name' => $data['field_name'],
    //));
  }
  return patterns_results($status, $msg, $result);
}

// Cleanup any global settings after the action runs
function node_patterns_cleanup($action, $tag, &$data) {
  // TODO
  // Make sure content info is always up-to-date
  //cache_clear_all('content_type_info', content_cache_tablename());// TODO
  _field_info_collate_types(true);

  return patterns_results();
}

