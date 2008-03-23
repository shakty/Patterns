<?php

function imagecache_patterns($op, $id = null, &$data = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('imagecache');
    break;
    
    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'imagecache_admin' => t('Setup Imagecache'),
      );
    break;
    
    // Prepare data for processing
    case 'prepare':
      $presets = array();
      
      // Ensure uniform structure as a single <preset> tag will create
      // a 'preset' key while multiple <preset> tags will create numeric
      // keys.
      if (!empty($data['preset'])) {
        for($i=0;$item = $data['preset'][$i]; $i++) {
          $data[$i] = $data['preset'][$i];
          unset($data['preset'][$i]);
        }
        
        if (empty($data['preset'])) {
          unset($data['preset']);
        }
      }
      
      for($index=0;$preset = $data[$index]; $index++) {
        if ($pid = _imagecache_preset_load_by_name($preset['name'])) {
          $preset['presetid'] = $pid;
        }
        else {
          $pid = 'new';
        }
        
        if (!empty($preset['action']) && !$preset['action'][0]) {
          $temp = $preset['action'];
          unset($preset['action']);
          $preset['action'][0] = $temp;
        }
        
        foreach((array)$preset['action'] as $i => $action) {
          $function = $action['function'] = $action['type'];
          unset($action['type']);
          
          $skip = array('data', 'actionid', 'presetid', 'weight');
          foreach($action as $key => $value) {
            if (in_array($key, $skip)) {
              continue;
            }
            else {
              $action['data'][$key] = $value;
              unset($action[$key]);
            }
          }
          
          if (!$action['weight']) {
            $action['weight'] = 0;
          }
          
          $preset['handlers']['new'][$function] = $action;
          unset($data[$index][$i], $preset[$i]);
        }
        
        if ($pid == 'new') {
          $presets[$pid][] = $preset;
        }
        else {
          $presets[$pid] = $preset;
        }
        
        unset($data[$index]);
      }

      unset($data['preset']);
      $data['presets'] = $presets;
    break;
    
    // Pre validate actions
    case 'pre-validate':
    break;
    
    // Return the form_id('s) for each action
    case 'form_id':
      return 'imagecache_admin';
    break;
    
    // Prepare for valid processing of this type of component
    case 'build':
      // First create any new presets here
      foreach((array)$data['presets']['new'] as $preset) {
        _imagecache_preset_create($preset['name']);
        $pid = db_result(db_query('SELECT id FROM {sequences} WHERE name = "{imagecache}_preset_presetid"'));
        $data['presets'][$pid] = $preset;
      }
      unset($data['presets']['new']);
      
      $presets = _imagecache_get_presets(true);
      
      // Then setup preset updating via imagecache module
      foreach($data['presets'] as $pid => $preset) {
        if (is_numeric($pid)) {
          // Delete old actions as they should be useless
          $actions = _imagecache_actions_get_by_presetid($pid);
          foreach($actions as $action) {
            _imagecache_action_delete($action);
          }
          
          // Create blank actions right here
          foreach($preset['handlers']['new'] as $handler => $action) {
            $new = array();
            $new['data'] = array('function' => $handler);
            $new['presetid'] = $pid;
            $new['weight'] = 0;
            _imagecache_action_create($new);
            
            $actionid = db_result(db_query('SELECT id FROM {sequences} WHERE name = "{imagecache_action}_actionid"'));
            $action['presetid'] = $pid;
            $action['actionid'] = $actionid;
            $data['presets'][$pid]['handlers'][$actionid] = $action;
          }
          unset($data['presets'][$pid]['handlers']['new']);
          
          if ($preset['delete']) {
            $_POST['preset-op'][$pid] = t('Delete preset');
          }
          else {
            $_POST['preset-op'][$pid] = t('Update preset');
          }
        }
      }
      
      return $data;
    break;
    
    // Validate the values for an action before running the pattern
    case 'validate':
      
    break;
    
    // Build a patterns actions and parameters
    case 'params':
    break;
    
    // Reverse actions when disabling a pattern
    case 'reverse':
      foreach($data as $pid => $preset) {
        if ($preset[$pid]['delete']) {
          unset($preset[$pid]['delete']);
        }
        else {
          $preset[$pid]['delete'] = true;
        }
      }
      return $data;
    break;
    
    // Cleanup any global settings after the action runs
    case 'cleanup':
      unset($_POST['preset-op'], $_REQUEST['destination']);
    break;
  }
}