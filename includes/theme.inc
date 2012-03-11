<?php
/**
 * @file
 * Theme related functions, hooks.
 */

/**
 * Implements hook_theme().
 */
function patterns_theme() {
  return array(
    'patterns_form_helper' => array(
      'arguments' => array('form_id' => NULL, 'values' => NULL),
    ),
    'patterns_form_helper_menu' => array(
      'forms' => NULL
    ),
    'patterns_list' => array(
      'variables' => array(
                     'patterns' => array(),
                     'invalid' => array(),
                     ),
    ),
    'patterns_patterns_fieldset' => array(
      'render element' => 'form',
    ),
  );
}

/**
 * @TODO Doc.
 *
 * @param $forms
 * @ingroup themeable
 */
function theme_patterns_form_helper_menu($forms) {
  $output = '<ul class="patterns-form-menu">';

  foreach ($forms as $form_id => $values) {
    $output .= '<li class="patterns-form-menu-item">' . $form_id . '</li>';
  }

  $output .= '</li>';

  return $output;
}

/**
 * @TODO Doc.
 *
 * @param unknown_type $form_id
 * @param unknown_type $values
 * @ingroup themeable
 */
function theme_patterns_form_helper($form_id, $values) {

  $output = '<div class="patterns-form" id="patterns-form-' . $form_id . '">';

  $output .= '<div class="patterns-form-title">' . t('Form values for %key', array('%key' => $form_id)) . '</div>';

  foreach ($values as $key => $value) {
    $output .= '<div class="patterns-form-item"><div class="patterns-form-key">' . $key . ' => </div>';
    $output .= '<div class="patterns-form-value">' . print_r($value, TRUE) . '</div></div>';
  }

  $output .= '</div>';

  return $output;
}

/**
 * Theme a list of patterns into separated fieldset ordered by category
 *
 * @param array $args An associative array containing: containing key 'patterns'
 * @ingroup themeable
 */
function theme_patterns_list($args) {

//  print_r($args);
//  exit;

  drupal_add_js('misc/collapse.js');
  drupal_add_js('misc/drupal.js');
  
  $patterns = $args[PATTERNS_STATUS_OK];
  $removed = $args[PATTERNS_STATUS_REMOVED];

  $patterns_title = '<div id="all_patterns_div"><h3>'. t('Available Patterns') .'</h3>';

  if (!file_exists(libraries_get_path('spyc') . '/spyc.php')) {
    $spyc = l(t('Spyc'), 'http://code.google.com/p/spyc/');
    $patterns_title .= t('!Spyc was not found. Please install the library in \'sites\all\libraries\spyc\' in order read YAML patterns.', array( '!Spyc' => $spyc ));
  }

  $form['patterns'] = array(
    '#prefix' => $patterns_title,
    '#suffix' => ' </div>',
    '#tree' => TRUE,
  );


  if (empty($patterns)) {
    $form['patterns']['#markup'] =  t('No patterns available.');
  }
  else {

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
            t('Name'),
            t('Title'),
            t('Version'),
            t('Description'),
          array('data' => t('Operations'), 'colspan' => 4),
        ),
        // Ensure that the "Core" package fieldset comes first.
        //'#weight' => $package == 'Core' ? -10 : NULL,
      );
    }
  }


  $form['removed'] = array(
    '#prefix' => '<div id="removed_patterns_div"><h3>' . t('Removed Patterns') . '</h3>',
    '#suffix' => ' </div>',
    '#tree' => TRUE,
  );

  if (empty($removed)) {
    $form['removed']['#markup'] = t('No patterns in the recycle bin.');
  }
  else {
    foreach ($removed as $pid => $pattern) {

      $category = $pattern->info['category'] ? $pattern->info['category'] : t('Other');

      $form['removed'][$category][] = _patterns_pattern_build_row($pid, $pattern);
      $form['removed'][$category] += array(
        '#type' => 'fieldset',
        '#title' => $category,
        '#collapsible' => TRUE,
        '#collapsed' => FALSE, 
        '#theme' => 'patterns_patterns_fieldset',
        '#header' => array(
          array('data' => t('Enabled'), 'class' => array('checkbox')),
            t('Name'),
            t('Title'),
            t('Version'),
            t('Description'),
          array('data' => t('Operations'), 'colspan' => 4),
        ),
        // Ensure that the "Core" package fieldset comes first.
        //'#weight' => $package == 'Core' ? -10 : NULL,
      );
    }
  }

  return drupal_render($form);
}


// Theming freely inspired by the Install Modules interface
// theme_system_modules_fieldset
// _system_modules_build_row

/**
 * Returns HTML for the modules form.
 *
 * @param $variables
 *   An associative array containing:
 *   - form: A render element representing the form.
 *
 * @ingroup themeable
 */
function theme_patterns_patterns_fieldset($variables) {
  $form = $variables['form'];

  // Individual table headers.
  $rows = array();
  // Iterate through all the modules, which are
  // children of this fieldset.
  foreach (element_children($form) as $key) {
    // Stick it into $pattern for easier accessing.
    $pattern = $form[$key];
    $row = array();
    unset($pattern['enable']['#title']);
    $row[] = array('class' => array('checkbox'), 'data' => drupal_render($pattern['enable']));
    $label = '<label ';
    if (isset($pattern['enable']['#id'])) {
      $label .= ' for="' . $pattern['enable']['#id'] . '"';
    }

    $row[] = $label . '><strong>' . drupal_render($pattern['name']) . '</strong></label>';

    $row[] = drupal_render($pattern['title']);
    
    $row[] = drupal_render($pattern['version']);
    // Add the description, along with any modules it requires.
    $description = drupal_render($pattern['description']);

    $row[] = array('data' => $description, 'class' => array('description'));

    // Display links (such as help or permissions) in their own columns.
    foreach ($pattern['links'] as $key => $value) {
      $row[] = array('data' => $value);
    }

    $rows[] = $row;
  }

  return theme('table', array('header' => $form['#header'], 'rows' => $rows));
}


/**
 * Build a table row for the tables in the pattern list page.
 *
 * @param mixed $pid the numeric id of the pattern as in the database
 * @param StdClass $pattern A pattern object as loaded from database
 * @param array $extra associative array of extra parameters. Not used now.
 */
function _patterns_pattern_build_row($pid, $pattern, $extra = array()) {
  // Add in the defaults.
  $extra = array(
    'requires' => array(),
    'required_by' => array(),
    'disabled' => FALSE,
    'enabled' => FALSE,
    'links' => array(),
  );

  $actions = array();

  // Build Run/Re-Run/Update pattern links
  if (!$pattern->status) {
    $actions['run'] =  l(t('Run'), 'admin/patterns/enable/' . $pid);
  }
  elseif ($pattern->enabled >= $pattern->updated) {
    $actions['run'] =  l(t('Re-Run'), 'admin/patterns/enable/' . $pid);
  }
  else {
    $actions['run'] = l(t('Run Update'), 'admin/patterns/enable/' . $pid);
  }

  // Edit Link
  $actions['edit'] = l(t('Edit'), 'admin/patterns/edit/' . $pid);

  // Remove Link
  //$actions['remove'] = l(t('Trash'), 'admin/patterns/trash/' . $pid, array('attributes' => array('class' => array('use-ajax'))));

  // Remove Link
  $actions['trash'] = l(t('Trash'), 'admin/patterns/trash/' . $pid, array('attributes' => array('class' => array('use-ajax'))));

   // Remove Link
  $actions['remove'] = l(t('Remove'), 'admin/patterns/remove/' . $pid);


  // Not yet ready
  //Publish/Unpublish link
  //if (variable_get('patterns_allow_publish', FALSE)) {
  //  $actions['publish'] = $pattern->public ? l(t('Unpublish'), 'admin/patterns/unpublish/' . $pid) : l(t('Publish'), 'admin/patterns/publish/' . $pid);
  //}

  $form = array(
    '#tree' => TRUE,
  );

  // Set the basic properties.

  // Creating overlay div with extra info
  $title = '<span id="pid-' . $pid . '" class="pattern-title">' . $pattern->title . '</span>';

  $info = array();
  $info[] = t('Author:') . ' ' . @$pattern->info['author'];
  $info[] = t('Email:') . ' ' . @$pattern->info['author_email'];
  $info[] = t('Web:') . ' ' . @$pattern->info['author_website'];
  $author = theme('item_list', array('items' => $info));

  $title .= '<div id="pid-' . $pid . '-info" class="pattern-info">' . $author . '</div>';

  $form['name'] = array(
    '#markup' => $pattern->name,
  );
  
  $form['title'] = array(
    '#markup' => $title,
  );
  $form['description'] = array(
    '#markup' => $pattern->description,
  );
  $form['version'] = array(
    '#markup' =>  @$pattern->info['version'],
  );
  $form['#requires'] = $extra['requires'];
  $form['#required_by'] = $extra['required_by'];

  // Check the compatibilities.
  $compatible = TRUE;
  $status_short = '';
  $status_long = '';

  //@TODO: do some checking about pattern compatibility
  $compatible = TRUE;

  // If this module is compatible, present a checkbox indicating
  // this module may be installed. Otherwise, show a big red X.
  if ($compatible) {
    $form['enable'] = array(
//      '#type' => 'checkbox',
//      '#title' => t('Enable'),
//      '#value' => $pattern->status,
//      //'#attributes' =>  array('checked' => $pattern->status),
//    );
//    if ($extra['disabled']) {
//      $form['enable']['#disabled'] = TRUE;
//    }
    '#markup' =>  ($pattern->status) ? t('Enabled') : t('Disabled'),
    );
  }
  else {
    $form['enable'] = array(
      '#markup' =>  theme('image', array('path' => 'misc/watchdog-error.png', 'alt' => $status_short, 'title' => $status_short)),
    );
    $form['description']['#markup'] .= theme('system_modules_incompatible', array('message' => $status_long));
  }

  $form['links'] = $actions;

  return $form;
}
