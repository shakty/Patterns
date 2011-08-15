<?php
/*
 * TODO: the region value somehow turns (string:"0") between block_admin_display_form and block_admin_display_form_submit
 * @file
 * Patterns component for blocks.
 */
function block_patterns($op, $id = NULL, &$data = NULL) {
  $defaults = array(
    'id'          => NULL, // May be a combo of module+delta
    'module'      => NULL,
    #'delta'       => NULL, // Always use isset with delta, because 0 is a legal value
    'description' => NULL,
    'info'        => NULL,
    'pages'       => NULL,
    'delete'      => NULL,
  );
  // Initialize the expected attributes to avoid PHP warnings. Makes later logic easier.
  $data = array_merge($defaults, (array)$data);

  switch ($op) {
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
      // Get the module and delta from the id, if provided
      if ($data['id']) {
        $split = strpos($data['id'], '-');
        if($split === FALSE) {
        	// TODO: check if "-" is present?
        }
        $data['module'] = substr($data['id'], 0, $split);
        $data['delta'] = substr($data['id'], $split + 1);

        unset($data['id']);
      }
      // Block description for custom blocks is always unique so we can try to get delta by looking up info field
      elseif ($data['info']) {
        $delta = db_query('SELECT delta FROM {block} bl INNER JOIN {block_custom} bo ON bl.delta = bo.bid AND bl.module = :module WHERE bo.info = :info', array('module' => 'block', 'info' => $data['info']))->fetchField();
        if ($delta) {
          $data['delta'] = $delta;
          $data['module'] = 'block';
        }
      }
      else {
      	// TODO: Error: you must provide either id or info.
      }

      if ($data['description']) { // TODO: Move this up before the previous block?
        $data['info'] = $data['description'];
        unset($data['description']);
      }

      if ($data['delete'] && !$data['info'] && $data['module'] == 'block') {
        $data['info'] = db_query('SELECT info FROM {block_custom} WHERE bid = :delta', array('delta' => $data['delta']))->fetchField();
      }

      if ($data['pages'] && is_array($data['pages'])) {
        $pages = implode("\r\n", $data['pages']);
        $data['pages'] = str_replace('[front]', '<front>', $pages);
      }

      if (!($data['module'] && isset($data['delta'])) && $data['info']) {
        $data['module'] = 'block';
      }
    break;

    // Pre validate actions
    case 'pre-validate':
      if (!($data['module'] && isset($data['delta'])) && !$data['info']) {
        return t('Missing required &lt;module&gt;, &lt;delta&gt; tags. Possibly malformed &lt;id&gt; tag could be the problem too. If creating a new block, tag &lt;info&gt; is required.');
      }
      elseif ($data['delete'] && $data['module'] != 'block') {
        return t('Unable to delete non-block module blocks.');
      }
    break;

    // Return the form_id('s) for each action
    case 'form_id':
      $diff = array_diff($data, _block_patterns_display_keys());

      // TODO: document, make this clearer
      if (!($data['module'] && $data['delta']) && $data['info']) {
        return array(
          'block_add_block_form',
          'block_admin_display_form'
        );
      }
      elseif ($data['delete']) {
        return 'block_box_delete';
      }
      elseif (empty($diff)) {
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

      if ($id == 'block_box_delete') { // TODO: no such form
        $data['op'] = t('Delete');
        $data['confirm'] = 1;
      }
      elseif ($id == 'block_admin_display_form') {
        if (isset($data['status']) && $data['status'] == 0) {
          $data['region'] = -1; // BLOCK_REGION_NONE, modules/block/block.module
        }

        // make sure delta is defined even for blocks that have just been created within the same action
        if ($data['module'] == 'block' && empty($data['delta'])) {
          // we can do this because block descriptions are always unique
          $data['delta'] = db_query('SELECT bid FROM {block_custom} WHERE info = ":info"', array('info' => $data['info']))->fetchField();
        }

        $data = array($data['module'] . '_' . $data['delta'] => $data);
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
      elseif ($id == 'block_admin_display_form') {
        $blocks = _block_rehash();
        $check = end($data); // this used to be current()

        // Only return the block we are dealing with now
        foreach ($blocks as $key => &$block) { // TODO: is there a function for this?
          if ($block['module'] == $check['module'] && $block['delta'] == $check['delta']) {
            $blocks = array(&$check); // NOTE: this used to be array(&$block)
            break;
          }
        }
        return array($blocks, $check['theme']);
      }
      elseif ($id == 'block_box_delete') {
        return $data['delta'];
      }
    break;
  }
}

function _block_patterns_display_keys() {
  return array('module', 'delta', 'theme', 'weight', 'region', 'status');
}
