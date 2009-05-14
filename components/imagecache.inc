<?php

function imagecache_patterns($op, $id = null, &$data = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('imagecache_preset', 'imagecache_action');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'imagecache_ui_preset_add_form' => t('Imagecache: Add preset'),
        'imagecache_ui_preset_form' => t('Imagecache: Edit preset'),
        'imagecache_ui_preset_flush_form' => t('Imagecache: Flush preset'),
        'imagecache_ui_preset_delete_form' => t('Imagecache: Delete preset'),
        'imagecache_ui_action_add_form' => t('Imagecache: Add action'),
        'imagecache_ui_action_form' => t('Imagecache: Configure action'),
        'imagecache_ui_action_delete_form' => t('Imagecache: Delete action'),
      );
    break;

    // Prepare data for processing
    case 'prepare':
      imagecache_presets(true);

      switch ($id) {
        case 'imagecache_preset':
          if (!empty($data['id'])) {
            $data['presetid'] = $data['id'];
            unset($data['id']);
          }

          if (!empty($data['name'])) {
            $data['presetname'] = $data['name'];
            unset($data['name']);
          }

          if (empty($data['presetid']) && !empty($data['presetname'])) {
            $presetid = db_result(db_query("SELECT presetid FROM {imagecache_preset} WHERE presetname = '%s'", $data['presetname']));
            if (is_numeric($presetid)) $data['presetid'] = $presetid;
          }

        break;
        case 'imagecache_action':

          if (is_numeric($data['id']) && empty($data['actionid'])) {
            $data['actionid'] = $data['id'];
            unset($data['id']);
          }

          if (is_string($data['name']) && empty($data['action'])) {
            $data['action'] = $data['name'];
            unset($data['name']);
          }

          if(empty($data['presetid']) && !empty($data['preset'])) {
            $presetid = db_result(db_query("SELECT presetid FROM {imagecache_preset} WHERE presetname = '%s'", $data['preset']));
            if (is_numeric($presetid)) {
              $data['presetid'] = $presetid;
              unset($data['preset']);
            }
          }

          if (empty($data['actionid']) && is_string($data['action'])) {
            $count = db_result(db_query("SELECT COUNT(*) FROM {imagecache_action} WHERE presetid = %d AND action = '%s'", $data['presetid'], $data['action']));
            if ($count == 1 && $actionid = db_result(db_query("SELECT actionid FROM {imagecache_action} WHERE presetid = %d AND action = '%s'", $data['presetid'], $data['action']))) {
              $data['actionid'] = $actionid;
            }
          }

          $skip = array('action', 'actionid', 'presetid', 'weight', 'delete', 'preset');
          foreach($data as $key => $value) {
            if (in_array($key, $skip)) continue;
            $values[$key] = $value;
            unset($data[$key]);
          }
          $data['data'] = $values;
        break;
      }
    break;

    // Pre validate actions
    case 'pre-validate':
      switch ($id) {
        case 'imagecache_preset':
          if ($data['delete'] && empty($data['presetid'])) {
            return t('You must specify valid preset.');
          }
          if (is_numeric($data['presetid'])) {
            $preset = imagecache_preset($data['presetid']);
            if (empty($preset)) {
              return t('Invalid preset id');
            }
            $data['old_presetname'] = $preset['presetname'];;
          }
          if (is_numeric($data['presetid']) && empty($data['presetname']) && !$data['delete']) {
            return t("presetname is missing.");
          }
        break;
        case 'imagecache_action':
          if (is_numeric($data['weight']) && ($data['weight'] < -10 || $data['weight'] > 10)) {
            return t('Weight should be a number between -10 and 10');
          }
          if (!empty($data['actionid']) && !imagecache_action_load($data['actionid'])) {
            return t('Invalid action ID: %id', array('%id' => $data['actionid']));
          }
        break;
      }
    break;

    // Return the form_id('s) for each action
    case 'form_id':
      switch ($id) {
        case 'imagecache_preset':
          if ($data['delete']) {
            return 'imagecache_ui_preset_delete_form';
          }
          else if ($data['flush']) {
            return 'imagecache_ui_preset_flush_form';
          }
          else if ($data['presetid'] && $data['presetname'] && $data['old_presetname'] != $data['presetname']) {
            return 'imagecache_ui_preset_form';
          }
          else if (empty($data['presetid'])) {
            return 'imagecache_ui_preset_add_form';
          }
        break;
        case 'imagecache_action':
          if ($data['delete']) {
            return 'imagecache_ui_action_delete_form';
          }
          else if (is_numeric($data['actionid']) && is_numeric($data['weight'])) {
            return array('imagecache_ui_action_form', 'imagecache_ui_preset_form');
          }
          else if (is_numeric($data['actionid'])) {
            return 'imagecache_ui_action_form';
          }
          else {
            return 'imagecache_ui_action_add_form';
          }
        break;
      }
    break;

    // Prepare for valid processing of this type of component
    case 'build':

      module_load_include('inc', 'imagecache', 'imagecache_ui.pages');

      if ($id == 'imagecache_ui_action_add_form') {
        unset($data['actionid']);
      }

      if ($id == 'imagecache_ui_preset_form' && is_numeric($data['actionid'])) {
        $preset = imagecache_preset($data['presetid'], TRUE);

        foreach ($preset['actions'] as $key => $action) {
          if ($action['actionid'] == $data['actionid']) {
            $preset['actions'][$key] = array_merge($preset['actions'][$key], $data);
          }
        }
        $data = array_merge($data, $preset);
        unset($data['weight'], $data['data'], $data['action'], $data['actionid']);
      }
      return $data;
    break;

    // Validate the values for an action before running the pattern
    case 'validate':
      if (strpos($id, 'ui_action')) {
        // try once again to determine presetid and actionid now that previous actions are executed
        // (this is required because all the actions are executed only after 'prepare' op so there
        //  is a good chance that required preset was still not created at that time)
        if(empty($data['presetid']) && !empty($data['preset'])) {
          $presetid = db_result(db_query("SELECT presetid FROM {imagecache_preset} WHERE presetname = '%s'", $data['preset']));
          if (is_numeric($presetid)) {
            $data['presetid'] = $presetid;
            unset($data['preset']);
          }
        }
        if (empty($data['actionid']) && is_string($data['action'])) {
          $count = db_result(db_query("SELECT COUNT(*) FROM {imagecache_action} WHERE presetid = %d AND action = '%s'", $data['presetid'], $data['action']));
          if ($count == 1 && $actionid = db_result(db_query("SELECT actionid FROM {imagecache_action} WHERE presetid = %d AND action = '%s'", $data['presetid'], $data['action']))) {
            $data['actionid'] = $actionid;
          }
        }
      }

      if ($id == 'imagecache_ui_action_add_form') {
        if (!is_numeric($data['presetid']) || !imagecache_preset($data['presetid'], TRUE)) {
          return t('Unknown preset.');
        }
      }
    break;

    // Build a patterns actions and parameters
    case 'params':
      if ($id == 'imagecache_ui_preset_delete_form' || $id == 'imagecache_ui_preset_flush_form' || $id == 'imagecache_ui_preset_form') {
        return array($data);
      }
      else if ($id == 'imagecache_ui_action_add_form') {
        return array(imagecache_preset($data['presetid']), $data['action']);
      }
      else if ($id == 'imagecache_ui_action_delete_form' || $id == 'imagecache_ui_action_form') {
        return array(imagecache_preset($data['presetid']), imagecache_action($data['actionid']));
      }
    break;

    // Cleanup any global settings after the action runs
    case 'cleanup':
      if (is_numeric($data['presetid'])) imagecache_preset_actions(array('presetid' => $data['presetid']), TRUE);
      imagecache_presets(TRUE);
    break;

    // Return the primary ID if possible from this action
    case 'identifier':
      if ($id == 'imagecache_ui_preset_add_form') {
        return db_result(db_query("SELECT presetid FROM {imagecache_preset} WHERE presetname = '%s'", $data['presetname']));
      }
      else if ($id == 'imagecache_ui_action_add_form') {

      }
    break;
  }
}
