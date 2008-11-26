<?php

function block_patterns($op, $id = null, &$data = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('block');
    break;
    
    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'block_admin_display_form' => t('Block region/display setup'),
        'block_admin_configure' => t('Configure a block'),
        'block_add_block_form' => t('Add new block'),
        'block_box_delete' => t('Delete a block')
      );
    break;
    
    // Prepare data for processing
    case 'prepare':
      if ($data['id']) {
        $split = explode('-', $data['id']);
        $data['module'] = $split[0];
        $data['delta'] = $split[1];
        
        unset($data['id']);
      }
      
      if ($data['description']) {
        $data['info'] = $data['description'];
        unset($data['description']);
      }
      
      if ($data['delete'] && $data['module'] == 'block') {
        $data['info'] = db_query('SELECT info FROM {boxes} WHERE bid = "%d"', $data['delta']);
      }
      
      if ($data['roles']) {
        foreach($data['roles'] as $key => $role) {
          if (is_string($role)) {
            $data['roles'][$key] = db_result(db_query('SELECT rid FROM {role} WHERE name = "%s"', $role));
          }
        }
      }
    break;
    
    // Pre validate actions
    case 'pre-validate':
      if (!(isset($data['module']) && isset($data['delta'])) && !$data['info']) {
        return t('Missing required &lt;module&gt;, &lt;delta&gt; tags. Possibly malformed &lt;id&gt; tag could be the problem too. If creating a new block, tag &lt;info&gt; is required.');
      }
      else if ($data['delete'] && $data['module'] != 'block') {
        return t('Unable to delete non-block module blocks');
      }
    break;
    
    // Return the form_id('s) for each action
    case 'form_id':
      $diff = array_diff($data, _block_patterns_display_keys());
      
      if (!($data['module'] && $data['delta']) && $data['info']) {
        return array(
          'block_add_block_form',
          'block_admin_configure'
        );
      }
      else if ($data['delete']) {
        return 'block_box_delete';
      }
      else if (empty($diff)) {
        return 'block_admin_display_form';
      }
      else if (count($diff) == count($data) - 2) {
        return 'block_admin_configure';
      }
      else {
        return array(
          'block_admin_display_form',
          'block_admin_configure'
        );
      }
    break;
    
    // Prepare for valid processing of this type of component
    case 'build':
      if ($id == 'block_box_delete') {
        $data['op'] = t('Delete');
        $data['confirm'] = 1;
      }
      else if ($id == 'block_admin_display_form') {
        if (isset($data['status']) && $data['status'] == 0) $data['region'] = -1;
        // Fetch and sort blocks
        $blocks = _block_rehash();
        module_load_include('inc', 'block', 'block.admin');
        usort($blocks, '_block_compare');
        $block = $data;
        $data = array();
        
        foreach($blocks as $i => $v) {
          if ($v['module'] == $block['module'] && $v['delta'] == $block['delta']) {
            $data[$block['module'] .'_'. $block['delta']] = $block;
//             dpm($data);
            break;
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
      if ($id == 'block_admin_configure') {
        return array($data['module'], $data['delta']);
      }
      else if ($id == 'block_admin_display_form') {
//         $blocks = _block_rehash();
//         module_load_include('inc', 'block', 'block.admin');
//         usort($blocks, '_block_compare');
        $b = reset($data);
//         dpm($blocks);
//         $blocks = $blocks + $data;
//         dpm($blocks);
        return array($data, $b['theme']);
      }
      else if ($id == 'block_box_delete') {
        return $data['delta'];
      }
    break;
    
    // Reverse actions when disabling a pattern
//     case 'reverse':
//     break;
  }
}

function _block_patterns_display_keys() {
  return array('module', 'delta', 'theme', 'weight', 'region');
}
