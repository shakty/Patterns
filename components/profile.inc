<?php

function profile_patterns($op, $id = null, &$data = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('profile');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'profile_field_form' => t('Profile: Add or edit field'),
        'profile_field_delete' => t('Profile: Delete field')
      );
    break;

    // Return a summary of an action
    case 'summary':
      if ($id == 'profile_field_form') {
        return t('Creating profile field %name', array('%name' => $data['title']));
      }
      else if ($id == 'profile_field_delete') {
        return t('Deleting profile field %name', array('%name' => $data['name']));
      }
    break;

    // Prepare data for processing
    case 'prepare':

      if (!empty($data['name'])) {
        $fid = db_result(db_query('SELECT fid FROM {profile_fields} WHERE name = "%s"', $data['name']));
      }

      if ($fid && !$data['fid']) {
        $data['fid'] = $fid;
      }

      if ($data['type'] == 'selection') {
        if (!empty($data['options']) && is_array($data['options'])) {
          $options = implode("\r\n", $data['options']);
          $data['options'] = $options;
        }
      }
      else {
        unset($data['options']);
      }
    break;

    // Pre validate actions
    case 'pre-validate':
      if (!$data['name']) {
        return t('"name" is required.');
      }
      if ($data['type'] == 'selection' && empty($data['options'])) {
        return t('"options" element is required for "selection" type of profile field.');
      }
    break;

    // Return the form_id('s) for each action
    case 'form_id':
      if ($data['delete']) {
        if (!$data['fid']) {
          return;
        }
        return 'profile_field_delete';
      }
      else {
        return 'profile_field_form';
      }
    break;

    // Prepare for valid processing of this type of component
    case 'build':
      module_load_include('inc', 'profile', 'profile.admin');
      if ($id == 'profile_field_delete') {
        $data['confirm'] = 1;
      }
      else if ($id == 'profile_field_form') {
        if ($data['fid']) {
          static $old_q;
          $old_q = $_GET['q'];
          $_GET['q'] = 'admin/user/profile/edit/'. $data['fid'];
        }
      }
      return $data;
    break;

    // Validate the values for an action before running the pattern
    case 'validate':
    break;

    // Build a patterns actions and parameters
    case 'params':
      if ($id == 'profile_field_form') {
        if (!$data['type'] && !$data['fid']) {
          return array('textfield');
        }
        else if (!$data['fid']) {
          return $data['type'];
        }
        else {
          return $data['fid'];
        }
      }
      else if ($id == 'profile_field_delete') {
        return $data['fid'];
      }
    break;

    // Cleanup any global settings or check created data
    case 'cleanup':
      static $old_q;

      if ($old_q) {
        $_GET['q'] = $old_q;
        unset($old_q);
      }
    break;
  }
}
