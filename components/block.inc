<?php
/*
 * TODO: The logic from 6.x to 7.x has changed a bit.
 *       When a theme is missing from "regions", it should be -1 and
 *       the corresponding block_admin_configure should not be called.
 *       On the other hand, for every theme supplied, the weight needs to be set properly.
 *       Also, try to avoid redundancy in the syntax of the pattern.
 * @file
 * Patterns component for blocks.
 */
function block_patterns($data) {

  $files = array('modules/block/block.admin.inc');

  $actions['block'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete Blocks'),
    PATTERNS_CREATE => array('block_admin_display_form', 'block_admin_configure', 'block_add_block_form'),
    PATTERNS_MODIFY => array('block_admin_display_form', 'block_admin_configure'),
    PATTERNS_DELETE => array('block_custom_block_delete'),
    PATTERNS_FILES  => $files,
  );

  $actions['block_extract_all'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete Blocks'),
    PATTERNS_MODIFY => array('block_admin_display_form', 'block_admin_configure'),
    PATTERNS_FILES  => $files,
  );

  return $actions;

}

function block_patterns_actions($data) {
  return array(
    'block_admin_display_form' => t('Block: Change the region or order of a block'),
    'block_admin_configure' => t('Block: Configure block'),
    'block_add_block_form' => t('Block: Add block'),
    'block_custom_block_delete' => t('Block: Delete custom block')
  );
}

// returns arguments needed for the forms when processing automatically
// $loop is set if we want to extract all values
function block_patterns_get_arguments($action, $tag, $form_id, &$loop = FALSE) {
  // gives one dataset as an example how it looks like, while extract_all
  // extracts all
  if ($tag == 'block' && $action == PATTERNS_MODIFY) {
    $block_list = _block_rehash();

    if ($form_id == 'block_admin_display_form') {
      $blocks = array();
      foreach ($block_list as $key => $block) {
        array_push($blocks, $block);
      }
      foreach (array_keys(list_themes()) as $theme) {
        return array($blocks, $theme);
      }
    }
    elseif ($form_id == 'block_admin_configure') {
      foreach ($block_list as $key => $block) {
        return array($block['module'], $block['delta']);
      }
    }

    return array();
  }
  elseif ($tag == 'block_extract_all') {
    $loop = TRUE;
    $block_list = _block_rehash();
    $return = array();

    if ($form_id == 'block_admin_display_form') {
      $blocks = array();
      foreach ($block_list as $key => $block) {
        array_push($blocks, $block);
      }
      foreach (array_keys(list_themes()) as $theme) {
        array_push($return, array($blocks, $theme));
      }
    }
    elseif ($form_id == 'block_admin_configure') {
      foreach ($block_list as $key => $block) {
        array_push($return, array($block['module'], $block['delta']));
      }
    }

    return $return;
  }
}

// Prepare data for processing
function block_patterns_prepare($action, $tag, &$data) {
  $defaults = array(
    'id'          => NULL, // May be a combo of module+delta
    'module'      => NULL,
    #'delta'       => NULL, // Always use isset with delta, because 0 is a legal value
    'description' => NULL,
    'info'        => NULL,
    'pages'       => NULL,
  );
  // Initialize the expected attributes to avoid PHP warnings. Makes later logic easier.
  $data = array_merge($defaults, (array) $data);

  if ($tag == 'block') {

    // Alias.
    if ($data['description']) {
      $data['info'] = $data['description'];
      unset($data['description']);
    }

    // Get the module and delta from the id, if provided.
    if ($data['id']) {
      $split = strpos($data['id'], '-');
      if ($split === FALSE) {
        // NOTE: No warning needed.
      }
      else {
        $data['module'] = substr($data['id'], 0, $split);
        $data['delta'] = substr($data['id'], $split + 1);
      }

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
      // TODO: Error: you must provide either id or info?
    }

    // The info field is needed for deletion.
    if ($action === PATTERNS_DELETE && !$data['info'] && $data['module'] == 'block' && $data['delta']) {
      $data['info'] = db_query('SELECT info FROM {block_custom} WHERE bid = :delta', array('delta' => $data['delta']))->fetchField();
      // TODO: check if it exists?
    }

    // Split the pages array into a multi-line string.
    if ($data['pages'] && is_array($data['pages'])) {
      $pages = implode("\r\n", $data['pages']);
      $data['pages'] = str_replace('[front]', '<front>', $pages);
    }

    $themes = array_keys(list_themes());

    // If the regions field is missing, assign -1 to every theme.
    if (!isset($data['regions']) or !is_array($data['regions'])
      or empty($data['regions'])) {
      $data['regions'] = array_combine($themes, array_fill(0, count($themes), '-1'));
    }
    else {
      // Replace empty or missing regions with "-1"
      foreach ($themes as $theme) {
        if (!isset($data['regions'][$theme]) or empty($data['regions'][$theme])) {
          $data['regions'][$theme] = -1;
        }
      }
      // TODO: check if the regions are all valid?
    }

    // This must be a custom block.
    if (!($data['module'] && isset($data['delta'])) && $data['info']) {
      $data['module'] = 'block';
    }

  }

  return patterns_results();
}

// Validate the values for an action before running the pattern
function block_patterns_validate($action, $tag, &$data) {
  $status = PATTERNS_SUCCESS;
  $msg = '';

  if ($tag == 'block') {
    if (!($data['module'] && isset($data['delta'])) && !$data['info']) {
      $status = PATTERNS_ERR;
      $msg = t('Missing required &lt;module&gt;, &lt;delta&gt; tags. Possibly malformed &lt;id&gt; tag could be the problem too. If creating a new block, tag &lt;info&gt; is required.');
    }
    elseif ($action === PATTERNS_DELETE && $data['module'] != 'block') {
      $status = PATTERNS_ERR;
      $msg = t('Unable to delete non-block module blocks.');
    }
    elseif ($action === PATTERNS_MODIFY || $action === PATTERNS_DELETE) {
      // Block must exist.
      // TODO: support $data['info']
      $block_count = db_query('SELECT COUNT(1) FROM {block} WHERE module = :module AND delta = :delta',
        array('module' => $data['module'], 'delta' => $data['delta']))->fetchField();
      if ($block_count == 0) {
        $status = PATTERNS_ERR;
        $msg = t('Block does not exist.');
      }
    }
  }

  return patterns_results($status, $msg);
}
// Return which callback functions to actually use.
function block_patterns_callbacks($action, $tag, &$data) {
  if ($tag == 'block') {
    $keys = array('module', 'delta', 'theme', 'weight', 'region', 'status');
    $diff = array_diff($data, $keys);

    // TODO: document, make this clearer.
    // Adding a new block: unknown id (or, module-delta) and a known info field.
    if ($action === PATTERNS_CREATE && !($data['module'] && $data['delta']) && $data['info']) {
      // $tag should be create
      $result = array(
          'block_add_block_form', // This will call block_admin_configure($form, $form_state, 'block', NULL);
          'block_admin_display_form'
        );
    }
    elseif ($action === PATTERNS_DELETE) { // Deleting a custom block.
      $result = array('block_custom_block_delete');
    }
    elseif (empty($diff)) { // Only editing the position for one theme.
      $result = array('block_admin_display_form'); // TODO: check _block_patterns_display_keys()
    }
    elseif ($action === PATTERNS_MODIFY) { // Editing a block
      $result = array(
          'block_admin_configure',
          'block_admin_display_form'
        );
    }
    else {
      // TODO
    }
  }

  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Prepare for valid processing of this type of component
function block_patterns_build($action, $form_id, &$data, &$a) {
  $status = PATTERNS_SUCCESS;
  $msg = '';
  $result = NULL;

  module_load_include('inc', 'block', 'block.admin'); // TODO: is this needed?
  // Fetch and sort blocks
  $blocks = _block_rehash();
  usort($blocks, '_block_compare');

  if ($form_id == 'block_custom_block_delete') {
    $data['op'] = t('Delete');
    $data['confirm'] = 1;
  }
  elseif ($form_id == 'block_admin_display_form') {
    if (isset($data['status']) && $data['status'] == 0) {
      $data['region'] = -1; // BLOCK_REGION_NONE, modules/block/block.module
    }

    // Make sure delta is defined even for blocks that have just been created within the same action.
    if ($data['module'] == 'block' && empty($data['delta'])) {
      // We can do this because block descriptions are always unique.
      $data['delta'] = db_query('SELECT bid FROM {block_custom} WHERE info = :info', array('info' => $data['info']))->fetchField();
    }

    $data = array($data['module'] . '_' . $data['delta'] => $data);
  }

  $result = $data;

  return patterns_results($status, $msg, $result);
}
// Build a patterns actions and parameters
function block_patterns_params($action, $form_id, &$data, &$a) {
  if ($form_id == 'block_admin_configure') {
    $result = array($data['module'], $data['delta']);
  }
  elseif ($form_id == 'block_admin_display_form') {
    $blocks = _block_rehash(); // TODO: Haven't we done this before?
    $check = end($data);

    // Only return the block we are dealing with now
    foreach ($blocks as $key => &$block) { // TODO: is there a function for this?
      if ($block['module'] == $check['module'] && $block['delta'] == $check['delta']) {
        $blocks = array(&$check);
        break;
      }
    }
    $result = array($blocks, $check['theme']);
  }
  elseif ($form_id == 'block_custom_block_delete') {
    $result = array('block', $data['delta']);
  }
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Cleanup any global settings after the action runs
function block_patterns_cleanup($action, $tag, &$data) {
  return patterns_results();
}
