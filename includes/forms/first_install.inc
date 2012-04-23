<?php
/**
 * @file
 * Displays a form of necessary input configuration to the user.
 */

function patterns_first_install_page() {

  module_load_include('inc', 'patterns', 'includes/forms/modules');


  //return patterns_forms_submodules();


  //$form['#submit'] = 'patterns_first_install_submit';

  //  return drupal_render($form);

  $out = drupal_get_form('patterns_first_install');
  return $out;
}


function patterns_first_install($form, &$form_state) {

  /*
  They can be executed manually or as a part of an automated web site deployment. There are main modules introduced as follow:
  System module - It provides basic but extensible functionality for use by other modules and themes.
  User module - It allows users with proper permissions to manage user roles (used to classify users) and permissions associated with those roles.
  Menu module - It is a hierarchical collection of links, which can be within or external to the site, generally used for navigation.
  Blocks module - They are the boxes visible in the sidebar(s) of your Drupal website.
  Field module - Custom data fields can be defined for entity types. The field data can be storing, loading, editing, and rendering.
    Taxonomy module - It allows you to classify the content of your website.
  Node module - The Node module manages the creation, editing, deletion, settings, and display of the main site content.
  Pathauto module - Pathauto provides a mechanism for modules to automatically generate aliases for the content they manage.
  Toolbar module - Toolbar module displays links to top-level administration menu items and links from other modules at the top of the screen.
  Shortcut module - It allows users to create sets of shortcut links to commonly-visited pages of the site.
  Color module - It allows users with the Administer site configuration permission to quickly and easily change the color scheme of themes that have been built to be compatible with it.';
  */

  $intro = 'This is the first time you installed patterns. The Patterns module is built to bypass this bottleneck by managing and automating site configuration. Site configuration is stored in YAML, XML, or PHP files called Patterns. These files have a structure which is easy to read, modify, manage, and share.They are portable, reusable, and extensible.';

  //$intro.= 'This page gives you a brief guide to the main functionalities of Patterns and settings. TODO check which parsers are enabled, if components are enabled, path to drush, and other settings.';

  $form['welcome'] = array(
    '#type' => 'fieldset',
    '#title' => t('Welcome to Patterns!'),
  );

  $form['welcome']['intro'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div><br/>',
    '#markup' => t($intro),
  );

  $parsers = patterns_parser_build_formats_index();
  if (empty($parsers)) {
    $form['welcome']['parsers'] = array(
      '#prefix' => '<strong>' . t('Warning:') . ' </strong>',
      '#markup' => t('It seems that you have not yet enabled any Patterns parser. You will not be able to load or run any pattern. Please visit the !modules page to enable at least one parser.', array('!modules' => l(t('modules'), 'admin/modules'))),
    );
  }

  $form['welcome']['drush_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Drush Path'),
    '#description' => t('If you have drush installed, you can automatically download missing modules necessary to run a pattern. Please specify the path. Otherwise leave it empty'),
    '#required' => FALSE,
    '#default_value' => variable_get('patterns_drush_path', ''),
  );

  $form['welcome']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Settings are OK'),
    '#suffix' => '<div>' . t('By pressing OK this page will not be shown any more. You can always modify your preferences from the \'Settings\' tab.') . '</div>',
  );

  return $form;
}

function patterns_first_install_validate($form, &$form_state) {
  return TRUE;
}

function patterns_first_install_submit($form, &$form_state) {
  variable_set(PATTERNS_FIRST_INSTALL, FALSE);
  variable_set('patterns_drush_path', $form_state['values']['drush_path']);
  menu_rebuild();
}
