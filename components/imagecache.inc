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
        'imagecache_ui_preset_add_form' => t('Add new preset'),
        'imagecache_ui_preset_form' => t('Edit preset'),
        'imagecache_ui_preset_flush_form' => t('Flush preset'),
        'imagecache_ui_preset_delete_form' => t('Delete preset'),
        'imagecache_ui_action_add_form' => t('Add action'),
        'imagecache_ui_action_form' => t('Edit action'),
        'imagecache_ui_action_delete_form' => t('Delete action'),
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
          
          if (is_numeric($data['id'])) {
            $data['actionid'] = $data['id'];
            unset($data['id']);
          }
          
          if (is_string($data['name']) && empty($data['action'])) {
            $data['action'] = $data['name'];
            unset($data['name']);
          }
       
//           if(empty($data['presetid']) && !empty($data['preset'])) {
//             $preset = imagecache_preset_by_name($data['preset']);
//             if (!empty($preset)) {
//               $data['presetid'] = $preset['presetid'];
//               unset($data['preset']);
//             }
//           }

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
          }
          if (is_numeric($data['presetid']) && empty($data['presetname'])) {
            return t("You didn't provide enough details about preset.");
          }
        break;
        case 'imagecache_action':
          if (is_numeric($data['weight']) && ($data['weight'] < -10 || $data['weight'] > 10)) {
            return t('Weight should be a number between -10 and 10');
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
          else if ($data['presetid'] && $data['presetname']) {
            return 'imagecache_ui_preset_form';
          }
          else {
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
//       if ($id == 'imagecache_ui_preset_delete_form' || $id == 'imagecache_ui_preset_delete_form') {
//         $data['confirm'] = 1;
//       }
      
      if ($id == 'imagecache_ui_action_add_form') {
        if(empty($data['presetid']) && !empty($data['preset'])) {
          $presetid = db_result(db_query("SELECT presetid FROM {imagecache_preset} WHERE presetname = '%s'", $data['preset']));
          if (is_numeric($presetid)) {
            $data['presetid'] = $presetid;
            unset($data['preset']);
          }
        }      
      }
      
      if ($id == 'imagecache_ui_preset_form' && is_numeric($data['actionid'])) {
        $d['presetid'] = $data['presetid'];
        $preset = imagecache_preset($data['presetid']);
        $d['presetname'] = $preset['presetname'];
        $d['actions'][0] = $data;
        $data = $d;
      }
      return $data;
    break;
    
    // Validate the values for an action before running the pattern
    case 'validate':
      
    break;
    
    // Build a patterns actions and parameters
    case 'params':
      if ($id == 'imagecache_ui_preset_delete_form' || $id == 'imagecache_ui_preset_flush_form' || $id == 'imagecache_ui_preset_form') {
        return array(imagecache_preset($data['presetid']));  
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
      imagecache_presets(true);
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
