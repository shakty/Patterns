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
        'block_admin_display_form' => t('Block: Change the region or order of a block'),
        'block_admin_configure' => t('Block: Configure block'),
        'block_add_block_form' => t('Block: Add block'),
        'block_box_delete' => t('Block: Delete block')
      );
    break;

    // Prepare data for processing
    case 'prepare':
      if ($data['id']) {
        $split = strpos($data['id'], '-');
        $data['module'] = substr($data['id'], 0, $split);
        $data['delta'] = substr($data['id'], $split + 1);

        unset($data['id']);
      }
      // Block description for custom blocks is always unique so we can try to get delta by looking up info field
      elseif (!empty($data['info'])) {
        $delta = db_result(db_query('SELECT delta FROM {blocks} bl INNER JOIN {boxes} bo ON bl.delta = bo.bid AND bl.module = "block" WHERE info = "%s"', $data['info']));
        if (!empty($delta)) {
          $data['delta'] = $delta;
          $data['module'] = 'block';
        }
      }

      if ($data['description']) {
        $data['info'] = $data['description'];
        unset($data['description']);
      }

      if ($data['delete'] && empty($data['info']) && $data['module'] == 'block') {
        $data['info'] = db_result(db_query('SELECT info FROM {boxes} WHERE bid = "%d"', $data['delta']));
      }

      if (!empty($data['pages']) && is_array($data['pages'])) {
        $pages = implode("\r\n", $data['pages']);
        $data['pages'] = str_replace('[front]', '<front>', $pages);
      }

      if (!($data['module'] && $data['delta']) && $data['info']) {
        $data['module'] = 'block';
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
          'block_admin_display_form'
        );
      }
      else if ($data['delete']) {
        return 'block_box_delete';
      }
      else if (empty($diff)) {
        return 'block_admin_display_form';
      }
      else {
        return array(
          'block_admin_configure',
          'block_admin_display_form'
        );
      }
    break;

    // Prepare for valid processing of this type of component
    case 'build':
      module_load_include('inc', 'block', 'block.admin');
      // Fetch and sort blocks
      $blocks = _block_rehash();
      usort($blocks, '_block_compare');

      if ($id == 'block_box_delete') {
        $data['op'] = t('Delete');
        $data['confirm'] = 1;
      }
      else if ($id == 'block_admin_display_form') {
        if (isset($data['status']) && $data['status'] == 0) {
          $data['region'] = -1;
        }

        // make sure delta is defined even for blocks that have just been created within the same action
        if ($data['module'] == 'block' && empty($data['delta'])) {
          // we can do this because block descriptions are always unique
          $data['delta'] = db_result(db_query("SELECT bid FROM {boxes} WHERE info = '%s'", $data['info']));
        }

        $data = array($data['module'] .'_'. $data['delta'] => $data);
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
        $blocks = _block_rehash();
        $check = current($data);
        // Only return the block we are dealing with now
        foreach($blocks as $key => &$block) {
          if ($block['module'] == $check['module'] && $block['delta'] == $check['delta']) {
            $blocks = array(&$block);
            break;
          }
        }
        return array($blocks, $check['theme']);
      }
      else if ($id == 'block_box_delete') {
        return $data['delta'];
      }
    break;
  }
}

function _block_patterns_display_keys() {
  return array('module', 'delta', 'theme', 'weight', 'region', 'status');
}
