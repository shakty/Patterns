<?php

function views_patterns($op, $id = null, &$data = null, $a = null) {
  static $forms, $form_data = array();

  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('view');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'views_ui_add_form' => t('Create view'),
        'views_ui_edit_view_form' => t('Edit/Save view'),
        'views_ui_edit_display_form' => t('Edit view display'),
        'views_ui_edit_details_form' => t('Edit view details'),
        'views_ui_add_item_form' => t('Add item to view'),
        'views_ui_config_type_form' => t('Configure a type'),
        'views_ui_config_item_form' => t('Configure an item'),
        'views_ui_config_item_extra_form' => t('Configure item extras'),
        'views_ui_rearrange_form' => t('Re-arrange view elements'),
        'views_ui_change_style_form' => t('Change style of view items'),
        'views_ui_config_style_form' => t('Configure view item styles'),
        'views_ui_delete_confirm' => t('Delete view'),
        'views_ui_remove_display_form' => t('Remove a display'),
        'enable_view' => t('Enable a disabled view'),
        'disable_view' => t('Disable an active view'),
        'clone_view' => t('Clone a view')
      );
    break;

    // Return a summary of an action
    // TODO: Finish the summaries
    case 'summary':
      // TODO add the rest of the form_ids here for summary
      $variables = array('%name' => $data['name']);
      switch($id) {
        case 'views_ui_add_form':
          return t('Create view %name', $variables);
        break;
        case 'views_ui_edit_view_form':
          return t('Edit/Save view %name', $variables);
        break;
        case 'views_ui_edit_display_form':
          return t('Edit view %name display', $variables);
        break;
        case 'views_ui_delete_confirm':
          return t('Delete view %name', $variables);
        break;
        case 'enable_view':
          return t('Enable view %name', $variables);
        break;
        case 'disable_view':
          return t('Disable view %name', $variables);
        break;
        case 'clone_view':
          return t('Clone view %name', $variables);
        break;
      }
    break;

    // Return the configuration object that ensures a well-formatted
    // object and determines how configurations should proceed
    case 'config':
      return array(
        '/display' => array('#array' => true, '#required' => true, '#create' => 'action', '#create value' => 'update'),
        '/display/*' => array('#key' => '/name', '#default key' => 'default'),
        '/display/details' => array('#required' => true),
        '/display/*/section' => array('#array' => true),
        '/display/*/section/*' => array('#create' => 'values', '#create value' => array()),
        '/display/*/section/*/section' => array('#required' => true),
        '/display/*/section/*/value' => array('#key' => '/../section', '#move' => '/../values'),
        '/display/*/item' => array('#array' => true),
        '/display/*/item/*' => array('#create' => 'action', '#create value' => 'update'),
        '/display/*/item/*/type' => array('#required' => true),
        '/display/*/item/*/id' => array('#required' => true),
        '/display/*/config' => array('#array' => true),
        '/display/*/style' => array('#array' => true),
        '/display/*/style/*/type' => array('#required' => true),
        '/display/*/style/*/name' => array('#required' => true)
      );
    break;

    // Prepare data for views-centric structure
    case 'prepare':
    break;

    // Validate and cleanup the values for an action before running the pattern
    case 'pre-validate':

      // TODO Require a type for new displays
      // TODO Require a name for views
      // TODO Check if this is not an enable/disable/clone/edit_details form and require a display
      if (!$data['details']) {
        return t('Missing required "details" element.');
      }
      if (!$data['details']['name'] && !$data['details']['vid']) {
        return t('Missing required views identifying "name" or "vid" element.');
      }
      if (!$data['display']) {
        return t('Missing required display element.');
      }
      if ($data['display']) {
        foreach($data['display'] as $key => $display) {
          if (!$key) {
            return t('Missing required name element inside of a display.');
          }
        }
      }
    break;

    // Ask the user if its ok to do certain tasks
    case 'confirmation form':
    break;

    // Validate action before processing
    case 'validate':
      // TODO validate using views_ui_ajax_forms that proper arguments are set for each form
    break;

    // Return the form id or determine if the action does not need processing
    case 'form_id':
      // Make sure necessary files are loaded.
      require_once(drupal_get_path('module', 'views') .'/includes/admin.inc');

      // Get a list of forms so we can have needed info for mapping and validating
      $forms = views_ui_ajax_forms();
      $form_ids = array();

      if ($data['action'] == 'delete') {
        $form_ids[] = 'views_ui_delete_confirm';
        $form_data[] = &$data['details'];
      }
      else if ($data['action'] == 'enable') {
        $form_ids[] = 'enable_view';
        $form_data[] = &$data['details'];
      }
      else if ($data['action'] == 'disable') {
        $form_ids[] = 'disable_view';
        $form_data[] = &$data['details'];
      }
      else if ($data['action'] == 'clone') {
        $form_ids[] = 'disable_view';
        $form_data[] = &$data['details'];
      }
      else {
        if (!($view = views_get_view($data['details']['vid'] ? $data['details']['vid'] : $data['details']['name']))) {
          $form_ids[] = 'views_ui_add_form';
          $form_data[] = &$data['details'];
        }
        else if(isset($data['details'])) {
          // Set the edit view details form
          $form_ids[] = 'views_ui_edit_details_form';
          $form_data[] = &$data['details'];
        }

        // Add any required displays
        $matches = patterns_array_fetch('/display/*', $data);
        foreach($matches as $match) {
          // Action is separate and not a display
          if ($match['key'] == 'action') {
            continue;
          }
          $action = $match['parent']['item']['action'];

          // Default display is always just default
          if ($match['key'] == 'default') {
          	$match['item']['display_id'] = $match['key'];
            continue;
          }

          // Check if we can update and a display exists
          if ($action != 'add') {
            foreach((array)$view->display as $key => $check) {
            	if ($check->display_plugin == $match['key']) {
            		$display = $check;
            	}
            }
            if ($display) {
            	$match['item']['display_id'] = $display->id;
            }
            else {
            	$form_ids[] = 'views_ui_add_display_form';
              $form_data[] = array('display' => $match['key']);
            }
          }
          else {
          	$form_ids[] = 'views_ui_add_display_form';
            $form_data[] = array('display' => $match['key']);
          }
        }

        // Use the patterns array fetch api to build the list of form_ids and their data
        $matches = patterns_array_fetch('/display/*/section/*', $data);
        foreach($matches as $match) {
          $parent = patterns_array_fetch('/../..', $match['item'], $match);
          // Make sure display_id value is set so it can be retreived later
          $parent[0]['item']['display_id'] = $parent[0]['item']['display_id'];
          $match['item']['display_id'] = &$parent[0]['item']['display_id'];
          $form_ids[] = 'views_ui_edit_display_form';
          $form_data[] = &$match['item'];
        }

        // Use the patterns array fetch api to build the list of form_ids and their data
        $matches = patterns_array_fetch('/display/*/style/*', $data);
        foreach($matches as $match) {
          $parent = patterns_array_fetch('/../..', $match['item'], $match);
          // Make sure display_id value is set so it can be retreived later
          $parent[0]['item']['display_id'] = $parent[0]['item']['display_id'];
          $match['item']['display_id'] = &$parent[0]['item']['display_id'];
          $form_ids[] = 'views_ui_change_style_form';
          $form_data[] = &$match['item'];
        }

        // Use the patterns array fetch api to build the list of form_ids and their data
        $matches = patterns_array_fetch('/display/*/config/*', $data);
        foreach($matches as $match) {
          $parent = patterns_array_fetch('/../..', $match['item'], $match);
          // Make sure display_id value is set so it can be retreived later
          $parent[0]['item']['display_id'] = $parent[0]['item']['display_id'];
          $match['item']['display_id'] = &$parent[0]['item']['display_id'];
          $form_ids[] = 'views_ui_config_type_form';
          $form_data[] = &$match['item'];
        }

        $matches = patterns_array_fetch('/display/*/item/*', $data);
        foreach($matches as $match) {
          $parent = patterns_array_fetch('/../..', $match['item'], $match);
          // Make sure display_id value is set so it can be retreived later
          $parent[0]['item']['display_id'] = $parent[0]['item']['display_id'];
          $match['item']['display_id'] = &$parent[0]['item']['display_id'];
          if ($match['item']['action'] == 'insert') {
            $form_ids[] = 'views_ui_add_item_form';
            $form_data[] = &$match['item'];
          }
          else if ($match['item']['action'] == 'update') {
            // If the view isn't created, then neither is this field
            // TODO Support adding a field and then updating it in a later <item> element
            if (!$view) {
              $form_ids[] = 'views_ui_add_item_form';
              $form_data[] = &$match['item'];
            }
            // Get a list of items and determine if there is one to update
            else if ($match['item']['id'] && !$view->get_item($parent[0]['key'], $match['item']['type'], $match['item']['id'])) {
              $form_ids[] = 'views_ui_add_item_form';
              $form_data[] = &$match['item'];
            }
            else {
              unset($match['item']['id']);
              $items = $view->get_items($match['item']['type'], $parent[0]['key']);
              $split = explode('.', $match['item']['name']);
              foreach($items as $key => $item) {
                if ($item['table'] == $split[0] && $item['field'] == $split[1]) {
                  $match['item']['id'] = $key;
                  break;
                }
              }

              if (!$match['item']['id']) {
                $form_ids[] = 'views_ui_add_item_form';
                $form_data[] = &$match['item'];
              }
            }
          }

          if ($match['item']['data']) {
            $form_ids[] = 'views_ui_config_item_form';
            $form_data[] = &$match['item'];
          }

          if ($match['item']['extras']) {
            $form_ids[] = 'views_ui_config_item_extra_form';
            $form_data[] = &$match['item'];
          }

          if ($match['item']['style']) {
            $form_ids[] = 'views_ui_config_style_form';
            $form_data[] = &$match['item'];
          }
        }

        // Set the remove display forms
        $matches = patterns_array_fetch('/display/*/action=remove', $data);
        foreach($matches as $match) {
          $form_ids[] = 'views_ui_remove_display_form';
          $form_data[] = &$match['parent']['item'];
        }

        $form_ids[] = 'views_ui_edit_view_form';
      }

      return $form_ids;
    break;

    // Add default values to the pattern where appropriate and return form data
    case 'build':
      // Get view object if available
      if ($data['details']['vid']) {
        $view = views_get_view($data['vid']);
      }
      else if ($data['details']['name']) {
        if (!($view = views_object_cache_get('view', $data['details']['name']))) {
          $view = views_get_view($data['details']['name']);
        }
      }

      if (!empty($form_data)) {
        $copy = array_shift($form_data);
      }
      else {
        $copy = &$data;
      }

      // TODO Test and split these forms out
      if (in_array($id, array('enable_view', 'disable_view', 'clone_view', 'views_ui_delete_confirm', 'views_ui_add_display_form'))) {
        return array(
          'storage' => null,
          'submitted' => false,
          'view' => $view,
          'values' => $copy
        );
      }
      else if ($id == 'views_ui_edit_details_form' || $id == 'views_ui_add_form') {
        return array(
          'storage' => null,
          'submitted' => false,
          'view' => $view,
          'values' => $copy
        );
      }
      else if ($id == 'views_ui_edit_view_form') {
        return array(
          'storage' => null,
          'submitted' => false,
          'view' => $view,
          'values' => array()
        );
      }
      else {
        $display_id = $copy['display_id'];

        // Build the arg and values list
        if ($id == 'views_ui_edit_display_form') {
          $args = array($copy['section']);
          $values = $copy['values'];
        }
        else if (in_array($id, array('views_ui_config_type_form', 'views_ui_rearrange_form', 'views_ui_add_item_form'))) {
          $args = array($copy['type']);
          if ($id == 'views_ui_add_item_form') {
            $values = array('name' => array($copy['name'] => $copy['name']));
          }
          else {
            $values = $copy;
          }
        }
        else if (in_array($id, array('views_ui_config_item_form', 'views_ui_config_item_extra_form', 'views_ui_change_style_form', 'views_ui_config_style_form'))) {
          if (!$copy['id']) {
            $items = $view->get_items($copy['type'], $display_id);
            $split = explode('.', $copy['name']);
            foreach($items as $key => $item) {
              if ($item['table'] == $split[0] && $item['field'] == $split[1]) {
                $copy['id'] = $key;
                break;
              }
            }
          }

          $args = array($copy['type'], $copy['id']);
          if ($id == 'views_ui_config_item_extra_form') {
            $values = array('options' => $copy['extras']);
          }
          else if ($id == 'views_ui_config_style_form') {
            $values = array('style_options' => $copy['style']);
          }
          else {
            $values = array('options' => $copy['data']);
          }
        }
        else {
          $args = array();
          $values = $copy;
        }

        // Determine the key for this particular form_id
        foreach($forms as $key => $form) {
          if ($form['form_id'] == $id) {
            break;
          }
        }

        return array('storage' => null, 'submitted' => false, 'values' => $values) + views_ui_build_form_state(false, $key, $view, $display_id, $args);
      }
    break;

    // Create extra parameters needed for a pattern execution
    case 'params':
      // The edit view form needs the view as a parameter as well
      if ($id == 'views_ui_edit_view_form') {
        return views_object_cache_get('view', $data['details']['vid'] ? $data['details']['vid'] : $data['details']['name']);
      }
    break;

    // Set new data needed for consequetive actions
    case 'cleanup':
      $form_state = &$a;
      // Set a displays id when adding one
      if ($id == 'views_ui_add_display_form') {
        $type = $a['values']['display'];
        foreach($form_state['view']->display as $key => $check) {
        	if ($check->display_plugin == $type) {
        		$display = $check;
        	}
        }
        if ($display) {
        	$data['display'][$type]['display_id'] = $display->id;
        }
      }
    break;

    // Return view identifiers
    case 'identifier':

    break;

    // Return the default values for an action
    case 'defaults':
    break;
  }
}