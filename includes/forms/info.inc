<?php
/**
 * @file
 * Functions, forms related to display general information
 * about the Patterns module as a whole.
 */

/**
 * Displays a summary of available parsers, components,
 * and patterns directories.
 *
 */
function patterns_info_page() {

  $form = array();

  $text = 'General Information about the state of the Patterns module.';
  $title = 'Patterns Info';
  patterns_forms_add_page_header($form, $title, $text);

  $form = patterns_info_parsers($form);
  $form = patterns_info_components($form);
  $form = patterns_info_directories($form);

  return drupal_render($form);
}

/**
 * Displays a summary of available Patterns directories
 *
 * @param array $form (optional) If specified, the form
 *   will be appended here
 *
 * @return array $form The form to be rendered
 */
function patterns_info_directories($form = array()) {
  $dirs = patterns_path_get_patterns_dirs();
  $dirs = patterns_array_to_rows($dirs);

  $header = array(
    t('N.'),
    t('Directory'),
  );

  $form['pd'] = array(
    '#type' => 'fieldset',
    '#title' => t('Patterns Directories'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('The following directories will be scanned for Patterns files.'),
  );

  $form['pd']['dirs'] = array(
    '#markup' => theme('table', array('header' => $header,
                                      'rows' => $dirs,
                                )),
  );

  return $form;
}

/**
 * Displays a summary of available Patterns components
 *
 * @param array $form (optional) If specified, the form
 *   will be appended here
 *
 * @return array $form The form to be rendered
 */
function patterns_info_components($form = array()) {

  $components = patterns_io_list_components();

  foreach ($components as $key => $value) {
    $name = basename($value);
    $components[$key] = array($name, $value);
  }

  $header = array(
    t('Name'),
    t('Path'),
  );


  $form['cd'] = array(
    '#type' => 'fieldset',
    '#title' => t('Patterns Components'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('The following Patterns components were found and are currently available.'),
  );



  $form['cd']['components'] = array(
    '#markup' => theme('table', array('header' => $header,
                                      'rows' => $components,
                            )),
  );

  return $form;
}

/**
 * Displays a summary of Patterns parsers
 *
 * @param array $form (optional) If specified, the form
 *   will be appended here
 *
 * @return array $form The form to be rendered
 */
function patterns_info_parsers($form = array()) {

  $parsers = patterns_parser_build_formats_index();

  foreach ($parsers as $key => $value) {
    $enabled = (patterns_utils_is_module_enabled($value)) ? t('Enabled') : t('Disabled');
    $parsers[$key] = array($enabled, $key, $value);
  }

  $header = array(
    t('Status'),
    t('Format'),
    t('Module'),
  );

  $form['parsers'] = array(
    '#type' => 'fieldset',
    '#title' => t('Patterns Parsers'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('Go to the !modules page to enable more Patterns parsers.', array('!modules' => l(t('modules'), 'admin/modules'))),
  );

  $form['parsers']['parsers'] = array(
    '#markup' => theme('table', array('header' => $header,
                                      'rows' => $parsers,
                                      'empty' => t('No valid parser found / enabled'),
                                      //'attributes' => array('class' => 'patterns-list'),
                              )),
  );

  return $form;
}
