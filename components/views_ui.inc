<?php
// $ID:  $

/**
 * @file patterns component for importing views directly from
 * the views_export dump.
 */


function views_ui_patterns($op, $id = null, &$data = null) {

  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('views_import');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'views_ui_import_page' => t('Views: Import a view from an exported view definition'),
        'views_ui_edit_view_form' => t('Views: Import a view from an exported view definition'),
      );
    break;

    // Prepare data for processing
    case 'prepare':
      if (!empty($data['filename']) && file_exists($data['filename'])) {
        $data['view'] = file_get_contents($data['filename']);
      }
      
      views_include('view');
      eval($data['view']);
      $data['view_object'] = $view;
      
      $data['name'] = !empty($data['name']) ? $data['name'] : $view->name;
    break;

    // Pre validate actions
    case 'pre-validate':      
      if (empty($data['view_object'])) {
        return t('Failed to find the view definition.');
      }
    break;

    // Return the form_id('s) for each action
    case 'form_id':
      return array(
        'views_ui_import_page',
        'views_ui_edit_view_form'
      );
    break;

    // Validate the values for an action before running the pattern
    case 'validate':
    break;

    // Prepare for valid processing of this type of component
    case 'build':
      views_include('admin');
      if ($id == 'views_ui_import_page') {
        $data['op'] = 'Import';
      }
      return $data;
    break;

    // Build a patterns actions and parameters
    case 'params':
      if ($id == 'views_ui_edit_view_form') {
        return array(views_ui_cache_load($data['name']));      
      }
    break;

    case 'cleanup':
      if ($id == 'views_ui_edit_view_form') {
        // Reset views static cache so that subsequent actions can 
        // get up to date list of existing views. 
        views_get_all_views(TRUE); 
      }
    break;

    case 'identifier':
    break;
  }
}
