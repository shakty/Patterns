<?php
/**
 * @file
 * Theme functions.
 */

// in progress
function patterns_list_patterns_2($patterns, $form = array(), $options = array()) {


  foreach ($patterns as $pid => $pattern) {

      $category = $pattern->info['category'] ? $pattern->info['category'] : t('Other');

      $form['patterns'][$category][] = _patterns_pattern_build_row($pid, $pattern);
      $form['patterns'][$category] += array(
        '#type' => 'fieldset',
        '#title' => $category,
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
        '#theme' => 'patterns_patterns_fieldset',
        '#header' => array(
          array('data' => t('Enabled'), 'class' => array('checkbox')),
            t('Format'),
            t('Name'),
            t('Title'),
            t('Version'),
            t('Description'),
            array('data' => t('Operations'), 'colspan' => 7),
        ),
        // Ensure that the "Core" package fieldset comes first.
        //'#weight' => $package == 'Core' ? -10 : NULL,
      );
  }
}



function patterns_get_patterns_links($pattern = NULL, $options = NULL) {
  $actions = array();
  $pattern = _patterns_db_get_pattern($pattern); // make sure we have an object
  if (!$pattern) return $actions;

  $pid = $pattern->pid;

  // Display full commands only in the admin pages
  if (!patterns_utils_is_public_page()) {
    // RUN

    // PATTERNS_STATUS_OK:
    $actions['run'] = t('Run');

    if ($pattern->status == PATTERNS_STATUS_ENABLED) {
      $actions['run'] = t('Re-Run');
    }
    elseif ($pattern->status == PATTERNS_STATUS_INVALID) {
      $actions['run'] = t('Invalid!');
    }
    elseif ($pattern->enabled >= $pattern->updated) {
      $actions['run'] = t('Run-Update');
    }

    if (!in_array($pattern->status, array(PATTERNS_STATUS_INVALID, PATTERNS_STATUS_TRASHED))) {
      $actions['run'] = l($actions['run'], 'admin/patterns/enable/' . $pid);
    }

    // EDIT

    $actions['edit'] = l(t('Edit'), 'admin/patterns/edit/' . $pid);

    // MODULES

    $actions['check'] = l(t('Check'), 'admin/patterns/check/' . $pid);

    // TRASH

    if ($pattern->status != PATTERNS_STATUS_TRASHED) {
      // Trash Link
      $actions['trash'] = l(t('Trash'), 'admin/patterns/trash/' . $pid, array('attributes' => array('class' => array('use-ajax'))));
    }
    else {
      $actions['trash'] = l(t('Restore'), 'admin/patterns/restore/' . $pid, array('attributes' => array('class' => array('use-ajax'))));
    }

    // REMOVE

    if (!empty($pattern->file) && is_writable($pattern->file)) {
      // Remove Link
      $actions['remove'] = l(t('Remove'), 'admin/patterns/remove/' . $pid);
    }
    else {
      $actions['remove'] = '-';
    }

    // PUBLISH

    if (variable_get('patterns_allow_publish', PATTERNS_ALLOW_PUBLISH_DEFAULT)) {
      $actions['publish'] = ($pattern->public) ? t('Unpublish') : t('Publish');
      if ($pattern->status != PATTERNS_STATUS_TRASHED) {
        $actions['publish'] = $pattern->public ? l($actions['publish'], 'admin/patterns/unpublish/' . $pid) : l($actions['publish'], 'admin/patterns/publish/' . $pid);
      }
    }
  }
  // GET

  $actions['uri'] = ($pattern->public) ? l(t('Get'), 'patterns/get/' . $pid, array( 'attributes' => array('target' => '_blank')))
                                       : '-';


  return $actions;
}

/**
 * Helper function to transform an array of items, into an array
 * of of arrays of items, so that the returned value can be themed
 * with theme_table.
 *
 * @param array $array An array of items
 * @param mixed $empty (optional) A string that will be added in case
 *   the array is empty. Defaults NULL
 *
 * @return array $array The transformed array
 */
function patterns_array_to_rows($array, $empty = NULL) {
  if (empty($array)) {
    $array = array(0 => array());
    if (!is_null($empty)) {
      $array[0][] = $empty;
    }
    return $array;
  }

  foreach ($array as $key => $value) {
    if (empty($key) || is_numeric($key)) {
      $array[$key] = array(($key+1), $value);
    }
    else {
      $array[$key] = array(($key), $value);
    }
  }
  return $array;
}
