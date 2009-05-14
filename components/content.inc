<?php

function content_patterns($op, $id = null, &$data = null, $a = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('content', 'field', 'display', 'group');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'node_type_form' => t('CCK: Create or Edit Content Type'),
        'node_type_delete_confirm' => t('CCK: Delete Content Type'),
        'content_field_overview_form' => t('CCK: Add Field or Group'),
        'content_field_remove_form' => t('CCK: Delete Field'),
        'content_field_edit_form' => t('CCK: Update Field'),
        'display'=> t("CCK: Configure field's display settings"),
        'fieldgroup_group_edit_form' => t('CCK: Edit Group'),
        'fieldgroup_remove_group' => t('CCK: Delete Group')
      );
    break;

    // Prepare the data
    case 'prepare':
      if ($id == 'field') {
        if ($data['name'] && !$data['field_name']) {
          $data['field_name'] = 'field_'. $data['name'];
          unset($data['name']);
        }

        $mappings = array(
          'type' => 'type_name',
          'widget' => 'widget_type',
          'option' => 'type'
        );
        foreach($mappings as $old => $new) {
          if (!empty($data[$old]) && empty($data[$new])) {
            $data[$new] = $data[$old];
            unset($data[$old]);
          }
        }

        if (!empty($data['group']) && !preg_match('/^group_/i', $data['group'])) {
          $data['group'] = 'group_'. $data['group'];
        }

        if (!empty($data['allowed_values']) && is_array($data['allowed_values'])) {
          $data['allowed_values'] = implode("\n", $data['allowed_values']);
        }
      }
      else if ($id == 'display') {

        $data['type'] = check_plain($data['type']);
        $data['field'] = check_plain($data['field']);
        if (empty($data['field']) && !empty($data['group'])) {
          $data['field'] = check_plain($data['group']);
          $data['is_group'] = true;
        }
        unset($data['group']);


        if (!$data['is_group'] && strpos($data['field'], 'field_') !== 0) {
          $data['field'] = 'field_'. $data['field'];
        }
        else if ($data['is_group'] && strpos($data['field'], 'group_') !== 0) {
          $data['field'] = 'group_'. $data['field'];
        }

        $build_modes_tabs = content_build_modes('_tabs');

        if (isset($data['rss'])) {
          if (is_string($data['rss'])) {
            $data[key($build_modes_tabs['rss']['build modes'])] = check_plain($data['rss']);
          }
          else if (is_array($data['rss'])) {
            if (!empty($data['rss']['format'])) {
              $data[key($build_modes_tabs['rss']['build modes'])]['format'] = check_plain($data['rss']['format']);
            }
            if (is_numeric($data['rss']['exclude'])) {
              $data[key($build_modes_tabs['rss']['build modes'])]['exclude'] = check_plain($data['rss']['exclude']);
            }
          }
          unset($data['rss']);
        }

        if (!empty($data['label'])) {
          if ($data['is_group']) {
            $data[$data['field']]['label'] = check_plain($data['label']);
          }
          else {
            $data[$data['field']]['label']['format'] = check_plain($data['label']);
          }
          unset($data['label']);
        }

        $build_modes_flat = content_build_modes();

        foreach ($data as $key => $value) {
          if (!isset($build_modes_flat[$key])) continue;
          if (is_string($value)) {
            $data[$data['field']][$key]['format'] = check_plain($value);
            $data[$data['field']][$key]['exclude'] = 0;
          }
          else if (is_array($value)) {
            if (!empty($value['format'])) {
              $data[$data['field']][$key]['format'] = check_plain($value['format']);
            }
            if (is_numeric($value['exclude'])) {
              $data[$data['field']][$key]['exclude'] = check_plain($value['exclude']);
            }
          }
          unset($data[$key]);
        }
      }
      else if ($id == 'group') {
        if ($data['name']) {
          $data['group_name'] = $data['name'];
          unset($data['name']);
        }

        if (empty($data['type_name']) && !empty($data['type'])) {
          $data['type_name'] = $data['type'];
          unset($data['type']);
        }

        if (!$data['group_name'] && $data['label']) {
          $data['group_name'] = strtolower(preg_replace('/[^a-zA-Z0-9_]/', '_', $data['label']));
        }

        if (!empty($data['group_name']) && !preg_match('/^group_/i', $data['group_name'])) {
          $data['group_name'] = 'group_'. $data['group_name'];
        }

        if (is_array($data['form'])) {
          $data['settings']['form'] = $data['form'];
          unset($data['form']);
        }
        else {
          if (!empty($data['style'])) {
            $data['settings']['form']['style'] = $data['style'];
            unset($data['style']);
          }
          if (!empty($data['form_description'])) {
            $data['settings']['form']['description'] = $data['form_description'];
            unset($data['form_description']);
          }
        }
        if (is_array($data['display'])) {
          $data['settings']['display'] = $data['display'];
          unset($data['display']);
        }
        else {
          if (!empty($data['display_description'])) {
            $data['settings']['display']['description'] = $data['display_description'];
            unset($data['display_description']);
          }
        }
      }
    break;

    // Validate the values for an action before running the pattern
    case 'pre-validate':
      if ($id == 'content') {
        $type = $data['type'];
        $name = $data['name'];

        if (!$type) {
          return t('&lt;type&gt; tag is missing.');
        }
      }
      else if ($id == 'field') {
        if (empty($data['type_name'])) {
          return t('&lt;type&gt; tag is missing.');
        }
        if (empty($data['field_name'])) {
          return t('&lt;name&gt; tag is missing.');
        }
      }
      else if ($id == 'group') {
        if (empty($data['type_name'])) {
          return t('&lt;type&gt; tag is missing.');
        }
        else if (preg_match('/[^a-z0-9_]/', $data['type_name'])) {
          return t('&lt;type&gt; tag is invalid.');
        }

        if (empty($data['group_name'])) {
          return t('&lt;name&gt; tag is missing.');
        }
      }
      else if ($id = 'display') {

        if (empty($data['type'])) {
          return t('&lt;type&gt; tag is missing.');
        }
        if (empty($data['field'])) {
          return t('&lt;field&gt; tag is missing.');
        }

        if (empty($data[$data['field']])) {
          return t("&lt;display&gt; doesn't contain any valid tags to process.");
        }
      }
    break;

    // Validate action before processing
    case 'validate':

        if (!empty($data['type_name']) || $id == 'display') {
          _content_type_info(true);
          $type_name = $id == 'display' ? $data['type'] : $data['type_name'];
          $content_type = content_types($type_name);
          if ($content_type['type'] != $type_name) {
            return t("Invalid tag &lt;type&gt;. '%type' is not a valid content type name or such content type doesn't exist.", array('%type' => $type_name));
          }
        }

        if ($id == 'content_field_remove_form') {
          static $field;

          if (!$field) {
            return t("Field %field doesn't exist within %type content type.", array('%field' => $data['field_name'], '%type' => $data['type_name']));
          }
        }
        else if ($id == 'content_field_overview_form') {
          if (!empty($data['group'])) {
            $groups = fieldgroup_groups($data['type_name'], FALSE, TRUE);
            if (!$groups[$data['group']]) {
              return t('Group %group is not available in %type content type.', array('%group' => $data['group'], '%type' => $data['type_name']));
            }
          }
        }
        else if ($id == 'display') {
          if ($data['is_group']) {
            $groups = fieldgroup_groups($data['type'], FALSE, TRUE);
            if (!$groups[$data['field']]) {
              return t('Group %group is not available in %type content type.', array('%group' => $data['field'], '%type' => $data['type']));
            }
          }
          else if (!content_fields($data['field'], $data['type'])) {
            return t("Field %field doesn't exist within %type content type.", array('%field' => $data['field'], '%type' => $data['type']));
          }
        }
    break;

    // Return the form id or determine if the action does not need processing
    case 'form_id':
      if ($id == 'content') {
        _content_type_info(true);
        $type = content_types($data['type']);
        if ($data['delete']) {
          if ($type['type'] == $data['type']) {
            return 'node_type_delete_confirm';
          }
        }
        else {
          return 'node_type_form';
        }
      }
      else if ($id == 'field') {
        // Make sure content type info is up-to-date
        _content_type_info(true);
        node_get_types('types', NULL, true);

        static $field;
        $field = content_fields($data['field_name'], $data['type_name']);

        if ($data['delete']) {
          return 'content_field_remove_form';
        }

        if ($field) {
          // update existing field
          if (empty($data['widget_type'])) {
             $data['widget_type'] = $field['widget']['type'];
          }

          // field type cannot be changed after the field was created
          // so set the correct value and prevent any potential errors
          $data['type'] = $field['type'];

          if (!empty($data['group'])) {
            $data[$data['field_name']] = array(
              'parent' => $data['group'],
              'hidden_name' => $data['field_name']
            );
            $forms[] = 'content_field_overview_form';
          }

          if ((!empty($data['label']) && $data['label'] != $field['widget']['label']) || (!empty($data['widget_type']) && $data['widget_type'] != $field['widget']['type'])) {
            $forms[] = 'content_field_basic_form';
          }

          $forms[] = 'content_field_edit_form';
          return $forms;

        }
        else {
          $field = content_fields($data['field_name']);
          // add existing field
          if ($field) {
            if (empty($data['widget_type'])) {
               $data['widget_type'] = $field['widget']['type'];
            }

            $data['type'] = $field['type'];

            if (empty($data['label'])) {
               $data['label'] = $field['widget']['label'];
            }

            $data['_add_existing_field'] = array(
              'label' => $data['label'],
              'field_name' => $data['field_name'],
              'widget_type' => $data['widget_type'],
              'hidden_name' => '_add_existing_field'
            );
            if (!empty($data['weight'])) {
              $data['_add_existing_field']['weight'] = $data['weight'];
            }
            if (!empty($data['group'])) {
              $data['_add_existing_field']['parent'] = $data['group'];
            }

            $forms[] = 'content_field_overview_form';

            $fields = array('type_name', 'label', 'field_name', 'type', 'widget_type', '_add_existing_field');
            $extra_fields = array_diff(array_keys($data), $fields);
            if (!empty($extra_fields)) {
              $forms[] = 'content_field_edit_form';
            }

            return $forms;
          }
          // add new field
          else {
            $data['_add_new_field'] = array(
              'label' => $data['label'],
              'field_name' => $data['field_name'],
              'type' => $data['type'],
              'widget_type' => $data['widget_type'],
              'hidden_name' => '_add_new_field'
            );

            if (!empty($data['weight'])) {
              $data['_add_new_field']['weight'] = $data['weight'];
              unset($data['weight']);
            }

            if (!empty($data['group'])) {
              $data['_add_new_field']['parent'] = $data['group'];
            }
            $forms[] = 'content_field_overview_form';

            $fields = array('type_name', 'label', 'field_name', 'type', 'widget_type', '_add_new_field');
            $extra_fields = array_diff(array_keys($data), $fields);
            if (!empty($extra_fields)) {
              $forms[] = 'content_field_edit_form';
            }

            return $forms;
          }
        }
      }
      else if ($id == 'display') {
        return 'display';
      }
      else if ($id == 'group') {
        cache_clear_all('fieldgroup_data', content_cache_tablename());
        $groups = fieldgroup_groups($data['type_name'], FALSE, TRUE);
        $group = $groups[$data['group_name']];

        if ($data['delete'] && $group) {
          return 'fieldgroup_remove_group';
        }
        else if (!$group && !$data['delete']) {

          $data['_add_new_group'] = array(
            'label' => $data['label'],
            'group_name' => $data['group_name'],
            'hidden_name' => '_add_new_group'
          );

          if (!empty($data['weight'])) {
            $data['_add_new_group']['weight'] = $data['weight'];
          }
          if (!empty($data['group_type'])) {
            $data['_add_new_group']['group_type'] = $data['group_type'];
          }
          if (!empty($data['group_option'])) {
            $data['_add_new_group']['group_option'] = $data['group_option'];
            unset($data['group_option']);
          }

          $forms[] = 'content_field_overview_form';

          if (!empty($data['settings'])) {
            $forms[] = 'fieldgroup_group_edit_form';
          }

          return $forms;
        }
        else if ($group) {
          return 'fieldgroup_group_edit_form';
        }
      }
    break;

    // Add default values to the pattern where appropriate and return form data
    case 'build':
      content_clear_type_cache();
      module_load_include('inc', 'content', 'includes/content.admin');

      if ($id == 'node_type_form') {

        module_load_include('inc', 'node', 'content_types');

        $type = $data['type'];
        $name = $data['name'];
        $types = content_types($type);

        if (!$types['type']) {
          // Set the name of the new content type if missing
          if (!$data['name']) {
            $data['name'] = $type;
          }

          $defaults = _node_type_set_defaults($data);
          $data = array_merge($defaults, $data);
        }

        $node_options = array('status', 'published', 'promote', 'moderate');
        $default_options = variable_get('node_options_'. $type, null);

        if (!empty($default_options)) {
          $data['node_options'] = array_combine($default_options, $default_options);
        }

        foreach($data as $key => $value) {
          if (in_array($key, $node_options)) {
            if ($value) {
              $data['node_options'][$key] = $key;
            }
            else {
              $data['node_options'][$key] = '';
            }
            unset($data[$key]);
          }
        }

        if (isset($data['has_body']) && !$data['has_body']) {
          $data['body_label'] = '';
        }
      }
      else if ($id == 'node_type_delete_confirm') {
        module_load_include('inc', 'node', 'content_types');
      }
      else if ($id == 'display') {

        if ($data['is_group']) {
          $groups = fieldgroup_groups($data['type'], FALSE, TRUE);
          $group = $groups[$data['field']];
          if ($group) {
            $group['settings']['display'] = $data[$data['field']] + $group['settings']['display'];
            fieldgroup_save_group($data['type'], $group);
          }
        }
        else {
          module_load_include('inc', 'content', 'includes/content.crud');
          $field = content_fields($data['field'], $data['type']);
          if ($field) {
            $field['display_settings'] = $data[$data['field']] + $field['display_settings'];
            content_field_instance_update($field);
          }
        }
        return t('Your settings have been saved.');
      }
      elseif ($id == 'content_field_overview_form') {
        // due to all the data being passed to multiple forms we may
        // end up with some CCK field's "title" setting overwriting title
        // field on this form with some bogus value
        if (!is_array($data['title'])) {
          unset($data['title']);
        }
      }
      return $data;
    break;

    // Create extra parameters needed for a pattern execution
    case 'params':
      if ($id == 'node_type_delete_confirm') {
        $type = (object)content_types($data['type']);
        return array($type);
      }
      else if ($id == 'node_type_form') {
        $type = (object)content_types($data['type']);
        return array($type);
      }
      else if ($id == 'content_field_edit_form' || $id == 'content_field_remove_form') {
        return array($data['type_name'], $data['field_name']);
      }
      else if ($id == 'content_field_basic_form') {
        return array($data);
      }
      else if ($id == 'content_field_overview_form') {
        return $data['type_name'];
      }
      else if ($id == 'fieldgroup_group_edit_form' || $id == 'fieldgroup_remove_group') {
        return array($data['type_name'], $data['group_name']);
      }
    break;

    // Any last cleanup items
    case 'cleanup':
      // Make sure content info is always up-to-date
      cache_clear_all('content_type_info', content_cache_tablename());
      _content_type_info(true);

      // make sure fieldgroup info is up to date
      cache_clear_all('fieldgroup_data', content_cache_tablename());
      if (module_exists('fieldgroup')) {
        fieldgroup_groups('', FALSE, TRUE);
      }
      // filter out confusing warning about 'allowed values' produced by optionwidgets module
      if ($id == 'content_field_edit_form' && strpos($data['widget_type'], 'optionwidget') === 0 && $warnings = drupal_get_messages('warning')) {
        foreach ($warnings['warning'] as $warning) {
          if ($warning != "You need to specify the 'allowed values' for this field.") {
            drupal_set_message($warning, 'warning');
          }
        }
      }
    break;
  }
}
