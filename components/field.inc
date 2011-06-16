<?php
function field_patterns($op, $id = NULL, &$data = NULL, $a = NULL) {
  switch ($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('field');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'field_ui_field_overview_form' => t('Add/Create a Fields'),
        //'field_ui_field_edit_form' => t('Define extra settings for Instances') // to edit a field already created
        'field_ui_field_settings_form' => t('Define extra settings for Instances')
      );
    break;

    // Prepare the data
    case 'prepare':
      
      //@TODO: separete preparation for different form
      //@TODO: delete data not used by subsequent forms
      if ($id == 'field') {

                
        // @TODO: we need to translate the value from the YAML file to to one accepted by the form
        // @TODO: field.inc : have good default values, eg: 'text_textfield'
        
        // FIELD
        
        $data['fields'] = array();
        
        $default_type = 'text'; //text_textfield';
        $default_parent = '';
        $default_weight = 1;
        $default_widget_type = 'text_textfield';
        $default_cardinality = 1;
        $fname = ''; // Init to a random value ??
        
        $default_storage_type = 'field_sql_storage';
        
        if (isset($data['label'])) {
          $data['fields']['_add_new_field']['label'] = $data['label'];
          unset($data['label']);
        }
        
        if (isset($data['name'])) {
           $data['fields']['_add_new_field']['field_name'] = $data['name']; 
           $fname = 'field_' . $data['name'];
           unset($data['name']);
        }
        
        
        if (isset($data['type'])) {
          $data['fields']['_add_new_field']['type'] = $data['type']; 
          unset($data['type']);
        }
        else {
          $data['fields']['_add_new_field']['type'] = $default_type;
        }
        
        
        // @TODO: check for incompatible widget_type and field_type
        
        // Term Reference:
        // taxonomy_autocomplete
        // options_buttons
        if (isset($data['widget'])) {
          
          switch ($data['widget']) {

            case 'autocomplete':
              $w = 'taxonomy_autocomplete';
              break;
            case 'buttons':
              $w = 'options_buttons';
              break;
          }
          $data['fields']['_add_new_field']['widget_type'] = $w;
          unset($data['widget']);
        }
        else {
          $data['fields']['_add_new_field']['widget_type'] =  $default_widget_type;
        }
        
        if (isset($data['weight'])) {
          $data['fields']['_add_new_field']['weight'] = $data['weight'];
          unset($data['weight']);
        }
        else {  
          $data['fields']['_add_new_field']['weight'] = $default_weight;
        }
        
        $data['fields']['_add_new_field']['parent'] = $default_parent;
        
        
        // INSTANCE
        
        // Set type-specific options.
        switch ($data['fields']['_add_new_field']['type']) {
          
          case 'taxonomy_term_reference':
            if (isset($data['vocabulary'])) {
              $data['field']['settings']['allowed_values'][0]['vocabulary'] = $data['vocabulary'];
              unset($data['vocabulary']);
            }
            else {
              //@TODO If not vocabulary selected pick up the first one
            }
        
        }  

        if (isset($data['cardinality'])) {
           //$c = array('und' => array(array('value' =>$data['cardinality'],'safe_value'=>$data['cardinality'])));
           $data['field']['cardinality'] = $data['cardinality'];
           unset($data['cardinality']);
        }
        else {
          $data['field']['cardinality'] = $default_cardinality;
        }
                
        // instance[required]
        if (isset($data['required']) && $data['required']) {
          $data['instance']['required'] = 1;
          unset($data['required']);
        }
        
        //instance[settings][user_register_form]
        if (isset($data['registration']) && $data['registration']) {
          $data['instance']['settings']['user_register_form'] = 1;
          unset($data['registration']);
        }
        
        if (isset($data['storage'])) {
          $data['instance']['settings']['storage']['type'] = $data['storage'];
          unset($data['storage']);
        }
        else {
          $data['instance']['settings']['storage']['type'] = $default_storage_type;
        }
        
        if (isset($data['default'])) {
          $data[$fname]['und'] = $data['default'];
          unset($data['default']); 
        }
         
        $data['instance']['widget']['weight'] = $data['fields']['_add_new_field']['weight'];
        $data['instance']['widget']['type'] = $data['fields']['_add_new_field']['widget_type'];
        $data['instance']['widget']['module'] = 'taxonomy';
        $data['instance']['widget']['active'] = 0;
        
        $data['instance']['display']['weight'] = 45;
        $data['instance']['display']['type'] = 'taxonomy_term_reference_link';
        $data['instance']['display']['module'] = 'taxonomy';
        $data['instance']['display']['label'] = 'above';
        $data['instance']['display']['settings'] = array();
        
        
        // Same laberl for instance and field
        $data['instance']['label'] = $data['fields']['_add_new_field']['label'];
        $data['instance']['field_name'] = $fname;  
        $data['instance']['entity_type'] = 'user';
        $data['instance']['bundle']  = 'user';
        
        if (isset($data['description'])) {
          $data['instance']['description'] = $data['description'];
          unset($data['description']);
        }    
        
        $data['instance']['default_value_widget']  = 'user';
        
        
        /*

        $instance Array [8] 
  [0...7] 
    field_name  field_urcaurcatirulero18  
    entity_type user  
    bundle  user  
    widget  Array [4] 
    label urcaurcatirulero17  
    required  1 
    description uippi 
    settings  Array [1] 
      [0...0] 
        user_register_form  1 
        
        
        $field  Array [2] 
  [0...1] 
    cardinality 4 
    settings  Array [1] 
      [0...0] 
        allowed_values  Array [1] 
          [0...0] 
            0 Array [2] 
              [0...1] 
                vocabulary  anothervoc  
                parent  0 

        
        
        */
        
        
      }
      
      
      /*
       * 
  
  
  values  Array [8] 
  [0...7] 
    instance  Array [8] 
      [0...7] 
        field_name  field_urcaurcatirulero14  
        entity_type user  
        bundle  user  
        widget  Array [4] 
        label urcaurcatirulero14  
        required  1 
        description dddddddddddddddd  
        settings  Array [1] 
          [0...0] 
            user_register_form  1 
    field_urcaurcatirulero14  Array [1] 
    field Array [2] 
      [0...1] 
        cardinality 9 
        settings  Array [1] 
          [0...0] 
            allowed_values  Array [1] 
              [0...0] 
                0 Array [2] 
                  [0...1] 
                    vocabulary  anothervoc  
                    parent  0 
    submit  Save settings 
    form_build_id form-QanhE_ceAaqd7KgYRhTz3_r3fIhcl5h7n4lva4cRcOY  
    form_token  sYPnxGLjTu_gPgeIzwNNofahRa9oNCJV-DpWQU2stYc 
    form_id field_ui_field_edit_form  
    op  Save settings 

  
       * 
       * 
       * 
       */
      
        
//        if ($data['name'] && !$data['field_name']) {
//          $data['field_name'] = 'field_'. $data['name'];
//          unset($data['name']);
//        }
//
//        $mappings = array(
//          'type' => 'type_name',
//          'widget' => 'widget_type',
//          'option' => 'type'
//        );
//        
//        foreach($mappings as $old => $new) {
//          if (!empty($data[$old]) && empty($data[$new])) {
//            $data[$new] = $data[$old];
//            unset($data[$old]);
//          }
//        }
//      }
    break;

    // Validate the values for an action before running the pattern
    case 'pre-validate':
//     
//     if ($id == 'field') {
//        if (empty($data['type_name'])) {
//          return t('&lt;type&gt; tag is missing.');
//        }
//        if (empty($data['field_name'])) {
//          return t('&lt;name&gt; tag is missing.');
//        }
//      }
  
    break;

    // Validate action before processing
    case 'validate':

       // nothing
        
    break;

    // Return the form id or determine if the action does not need processing
    case 'form_id':
      if ($id == 'field') {
  
        return array(
                      'field_ui_field_overview_form',
                      //'field_ui_field_edit_form'
                      'field_ui_field_settings_form',
                      'field_ui_field_settings_form'
                     );
        
      }
      
    break;

    // Add default values to the pattern where appropriate and return form data
    case 'build':
      //content_clear_type_cache();
      module_load_include('inc', 'field_ui', 'field_ui.admin');
      
/*      
      
 input  Array [7] 
  [0...6] 

    fields  Array [1] 
      [0...0] 
        _add_new_field  Array [6] 
          [0...5] 
            label urcaurcatirulero15  
            field_name  urcaurcatirulero15  
            type  taxonomy_term_reference 
            widget_type taxonomy_autocomplete 
            weight  -10 
            parent  
            
            
            
    field Array [2] 
      [0...1] 
        settings  Array [1] 
          [0...0] 
            allowed_values  Array [1] 
        cardinality 5 
        
        
    instance  Array [3] 
      [0...2] 
       ----- required  1 
        settings  Array [2] 
          [0...1] 
            user_register_form  1 
            storage Array [1] 
      -------  description Stefano is cool 
    
    
    field_urcaurcatirulero15  Array [1] 
      [0...0] 
        und Economics 


input Array [7] 
  [0...6] 
    instance  Array [4] 
      [0...3] 
        label urcaurcatirulero13  *********
        required  1 
        description dede  
        settings  Array [1] 
        
    field_urcaurcatirulero13  Array [1] 
      [0...0] 
        und Economics 
        
        
    field Array [2] 
      [0...1] 
        cardinality 6 
        settings  Array [1] 
          [0...0] 
            allowed_values  Array [1] 
              [0...0] 
                0 Array [1] 
                  [0...0] 
                    vocabulary  anothervoc  
   
      
      field Array [1] 
  [0...0] 
    field_urcaurcatirulero12  Array [1] 
      [0...0] 
        und Array [5] 
          [0...4] 
            field Array [16]  
              [0...15]  
                translatable  1 
                entity_types  Array [0] 
                settings  Array [1] 
                storage Array [5] 
                foreign keys  Array [1] 
                indexes Array [1] 
                id  104 
                field_name  field_urcaurcatirulero12  
                type  taxonomy_term_reference 
                module  taxonomy  
                active  1 
                locked  0 
                cardinality 10  
                deleted 0 
                columns Array [1] 
                bundles Array [1] 
            instance  Array [13]  
              [0...12]  
            items_count 1 
            array_parents Array [4] 
            errors  Array [0] 
      
      */
      
//      $form_state Array [13]  
//  [0...12]  
//    build_info  Array [2] 
//      [0...1] 
//        args  Array [2] 
//          [0...1] 
//            0 user  
//            1 user  
//        files Array [1] 
//          [0...0] 
//            menu  modules/field_ui/field_ui.admin.inc 
//    rebuild false 
//    rebuild_info  Array [0] 
//    redirect  <Uninitialized> 
//    temporary Array [0] 
//    submitted false 
//    executed  false 
//    programmed  false 
//    cache false 
//    method  post  
//    groups  Array [0] 
//    buttons Array [0] 
//    input Array [5] 
//      [0...4] 
//        fields  Array [10]  
//          [0...9] 
//            account Array [3] 
//            field_ullalah Array [3] 
//            field_stefanoiscool Array [3] 
//            timezone  Array [3] 
//            profyle_this_is_new_field Array [3] 
//            field_sterel  Array [3] 
//            field_science_discipline  Array [3] 
//            profyle_  Array [3] 
//            _add_new_field  Array [7] 
//              [0...6] 
//                label proviamounnuovo 
//                weight  1 
//                parent  
//                parent_wrapper  Array [1] 
//                field_name  prunnuo 
//                type  text  
//                widget_type text_textfield  
//            _add_existing_field Array [5] 
//        form_build_id form-2xW-DQZZDFiX8z0qTGxZm9u4iiy4D_W6rU5allBIhyo  
//        form_token  DgxOX-XuYgd2-EYSMVGmKtHTOkwrakqnYOK1mmhUHCs 
//        form_id field_ui_field_overview_form  
//        op  Save  
      
      
      
      
      return $data;
      
    break;

    // Create extra parameters needed for a pattern execution
    case 'params':
        
      if ($id == 'field_ui_field_overview_form') {
        return array('user', 'user');
      }
      //else if ($id == 'field_ui_field_edit_form' ) {
      elseif ($id == 'field_ui_field_settings_form' ) {
        
        $fname = 'field_' . $data['fields']['_add_new_field']['field_name'];
        $instance = db_query("SELECT * FROM {field_config_instance} WHERE field_name = :fname", array('fname' => $fname))->fetchAssoc();
        return array($instance);
      }
      
    break;

    // Any last cleanup items
    case 'cleanup':
      // Make sure content info is always up-to-date
//      cache_clear_all('content_type_info', content_cache_tablename());
//      _content_type_info(true);
//
//      // make sure fieldgroup info is up to date
//      cache_clear_all('fieldgroup_data', content_cache_tablename());
//      if (module_exists('fieldgroup')) {
//        fieldgroup_groups('', FALSE, TRUE);
//      }
//      // filter out confusing warning about 'allowed values' produced by optionwidgets module
//      if ($id == 'content_field_edit_form' && strpos($data['widget_type'], 'optionwidget') === 0 && $warnings = drupal_get_messages('warning')) {
//        foreach ($warnings['warning'] as $warning) {
//          if ($warning != "You need to specify the 'allowed values' for this field.") {
//            drupal_set_message($warning, 'warning');
//          }
//        }
//      }
    break;
  }
  
//function field_patterns($op, $id = null, &$data = null, $a = null) {
//  
//  switch ($op) {
//    // Return the valid tags that this component can prepare and process
//    case 'tags':
//      return array('content', 'field', 'display', 'group');
//    break;
//
//    // Return a list of forms/actions this component can handle
//    case 'actions':
//      return array(
//        'node_type_form' => t('CCK: Create or Edit Content Type'),
//        'node_type_delete_confirm' => t('CCK: Delete Content Type'),
//        'content_field_overview_form' => t('CCK: Add Field or Group'),
//        'content_field_remove_form' => t('CCK: Delete Field'),
//        'content_field_edit_form' => t('CCK: Update Field'),
//        'display'=> t("CCK: Configure field's display settings"),
//        'fieldgroup_group_edit_form' => t('CCK: Edit Group'),
//        'fieldgroup_remove_group' => t('CCK: Delete Group')
//      );
//    break;
//
//    // Prepare the data
//    case 'prepare':
//      if ($id == 'field') {
//        if ($data['name'] && !$data['field_name']) {
//          $data['field_name'] = 'field_'. $data['name'];
//          unset($data['name']);
//        }
//
//        $mappings = array(
//          'type' => 'type_name',
//          'widget' => 'widget_type',
//          'option' => 'type'
//        );
//        foreach($mappings as $old => $new) {
//          if (!empty($data[$old]) && empty($data[$new])) {
//            $data[$new] = $data[$old];
//            unset($data[$old]);
//          }
//        }
//
//        if (!empty($data['group']) && !preg_match('/^group_/i', $data['group'])) {
//          $data['group'] = 'group_'. $data['group'];
//        }
//
//        if (!empty($data['allowed_values']) && is_array($data['allowed_values'])) {
//          $data['allowed_values'] = implode("\n", $data['allowed_values']);
//        }
//      }
//      elseif ($id == 'display') {
//
//        $data['type'] = check_plain($data['type']);
//        $data['field'] = check_plain($data['field']);
//        if (empty($data['field']) && !empty($data['group'])) {
//          $data['field'] = check_plain($data['group']);
//          $data['is_group'] = true;
//        }
//        unset($data['group']);
//
//
//        if (!$data['is_group'] && strpos($data['field'], 'field_') !== 0) {
//          $data['field'] = 'field_'. $data['field'];
//        }
//        elseif ($data['is_group'] && strpos($data['field'], 'group_') !== 0) {
//          $data['field'] = 'group_'. $data['field'];
//        }
//
//        $build_modes_tabs = content_build_modes('_tabs');
//
//        if (isset($data['rss'])) {
//          if (is_string($data['rss'])) {
//            $data[key($build_modes_tabs['rss']['build modes'])] = check_plain($data['rss']);
//          }
//          elseif (is_array($data['rss'])) {
//            if (!empty($data['rss']['format'])) {
//              $data[key($build_modes_tabs['rss']['build modes'])]['format'] = check_plain($data['rss']['format']);
//            }
//            if (is_numeric($data['rss']['exclude'])) {
//              $data[key($build_modes_tabs['rss']['build modes'])]['exclude'] = check_plain($data['rss']['exclude']);
//            }
//          }
//          unset($data['rss']);
//        }
//
//        if (!empty($data['label'])) {
//          if ($data['is_group']) {
//            $data[$data['field']]['label'] = check_plain($data['label']);
//          }
//          else {
//            $data[$data['field']]['label']['format'] = check_plain($data['label']);
//          }
//          unset($data['label']);
//        }
//
//        $build_modes_flat = content_build_modes();
//
//        foreach ($data as $key => $value) {
//          if (!isset($build_modes_flat[$key])) continue;
//          if (is_string($value)) {
//            $data[$data['field']][$key]['format'] = check_plain($value);
//            $data[$data['field']][$key]['exclude'] = 0;
//          }
//          elseif (is_array($value)) {
//            if (!empty($value['format'])) {
//              $data[$data['field']][$key]['format'] = check_plain($value['format']);
//            }
//            if (is_numeric($value['exclude'])) {
//              $data[$data['field']][$key]['exclude'] = check_plain($value['exclude']);
//            }
//          }
//          unset($data[$key]);
//        }
//      }
//      elseif ($id == 'group') {
//        if ($data['name']) {
//          $data['group_name'] = $data['name'];
//          unset($data['name']);
//        }
//
//        if (empty($data['type_name']) && !empty($data['type'])) {
//          $data['type_name'] = $data['type'];
//          unset($data['type']);
//        }
//
//        if (!$data['group_name'] && $data['label']) {
//          $data['group_name'] = strtolower(preg_replace('/[^a-zA-Z0-9_]/', '_', $data['label']));
//        }
//
//        if (!empty($data['group_name']) && !preg_match('/^group_/i', $data['group_name'])) {
//          $data['group_name'] = 'group_'. $data['group_name'];
//        }
//
//        if (is_array($data['form'])) {
//          $data['settings']['form'] = $data['form'];
//          unset($data['form']);
//        }
//        else {
//          if (!empty($data['style'])) {
//            $data['settings']['form']['style'] = $data['style'];
//            unset($data['style']);
//          }
//          if (!empty($data['form_description'])) {
//            $data['settings']['form']['description'] = $data['form_description'];
//            unset($data['form_description']);
//          }
//        }
//        if (is_array($data['display'])) {
//          $data['settings']['display'] = $data['display'];
//          unset($data['display']);
//        }
//        else {
//          if (!empty($data['display_description'])) {
//            $data['settings']['display']['description'] = $data['display_description'];
//            unset($data['display_description']);
//          }
//        }
//      }
//    break;
//
//    // Validate the values for an action before running the pattern
//    case 'pre-validate':
//      if ($id == 'content') {
//        $type = $data['type'];
//        $name = $data['name'];
//
//        if (!$type) {
//          return t('&lt;type&gt; tag is missing.');
//        }
//      }
//      elseif ($id == 'field') {
//        if (empty($data['type_name'])) {
//          return t('&lt;type&gt; tag is missing.');
//        }
//        if (empty($data['field_name'])) {
//          return t('&lt;name&gt; tag is missing.');
//        }
//      }
//      elseif ($id == 'group') {
//        if (empty($data['type_name'])) {
//          return t('&lt;type&gt; tag is missing.');
//        }
//        elseif (preg_match('/[^a-z0-9_]/', $data['type_name'])) {
//          return t('&lt;type&gt; tag is invalid.');
//        }
//
//        if (empty($data['group_name'])) {
//          return t('&lt;name&gt; tag is missing.');
//        }
//      }
//      elseif ($id = 'display') {
//
//        if (empty($data['type'])) {
//          return t('&lt;type&gt; tag is missing.');
//        }
//        if (empty($data['field'])) {
//          return t('&lt;field&gt; tag is missing.');
//        }
//
//        if (empty($data[$data['field']])) {
//          return t("&lt;display&gt; doesn't contain any valid tags to process.");
//        }
//      }
//    break;
//
//    // Validate action before processing
//    case 'validate':
//
//        if (!empty($data['type_name']) || $id == 'display') {
//          _content_type_info(true);
//          $type_name = $id == 'display' ? $data['type'] : $data['type_name'];
//          $content_type = content_types($type_name);
//          if ($content_type['type'] != $type_name) {
//            return t("Invalid tag &lt;type&gt;. '%type' is not a valid content type name or such content type doesn't exist.", array('%type' => $type_name));
//          }
//        }
//
//        if ($id == 'content_field_remove_form') {
//          static $field;
//
//          if (!$field) {
//            return t("Field %field doesn't exist within %type content type.", array('%field' => $data['field_name'], '%type' => $data['type_name']));
//          }
//        }
//        elseif ($id == 'content_field_overview_form') {
//          if (!empty($data['group'])) {
//            $groups = fieldgroup_groups($data['type_name'], FALSE, TRUE);
//            if (!$groups[$data['group']]) {
//              return t('Group %group is not available in %type content type.', array('%group' => $data['group'], '%type' => $data['type_name']));
//            }
//          }
//        }
//        elseif ($id == 'display') {
//          if ($data['is_group']) {
//            $groups = fieldgroup_groups($data['type'], FALSE, TRUE);
//            if (!$groups[$data['field']]) {
//              return t('Group %group is not available in %type content type.', array('%group' => $data['field'], '%type' => $data['type']));
//            }
//          }
//          elseif (!content_fields($data['field'], $data['type'])) {
//            return t("Field %field doesn't exist within %type content type.", array('%field' => $data['field'], '%type' => $data['type']));
//          }
//        }
//    break;
//
//    // Return the form id or determine if the action does not need processing
//    case 'form_id':
//      if ($id == 'content') {
//        _content_type_info(true);
//        $type = content_types($data['type']);
//        if ($data['delete']) {
//          if ($type['type'] == $data['type']) {
//            return 'node_type_delete_confirm';
//          }
//        }
//        else {
//          return 'node_type_form';
//        }
//      }
//      elseif ($id == 'field') {
//        // Make sure content type info is up-to-date
//        _content_type_info(true);
//        node_get_types('types', NULL, true);
//
//        static $field;
//        $field = content_fields($data['field_name'], $data['type_name']);
//
//        if ($data['delete']) {
//          return 'content_field_remove_form';
//        }
//
//        if ($field) {
//          // update existing field
//          if (empty($data['widget_type'])) {
//             $data['widget_type'] = $field['widget']['type'];
//          }
//
//          // field type cannot be changed after the field was created
//          // so set the correct value and prevent any potential errors
//          $data['type'] = $field['type'];
//
//          if (!empty($data['group'])) {
//            $data[$data['field_name']] = array(
//              'parent' => $data['group'],
//              'hidden_name' => $data['field_name']
//            );
//            $forms[] = 'content_field_overview_form';
//          }
//
//          if ((!empty($data['label']) && $data['label'] != $field['widget']['label']) || (!empty($data['widget_type']) && $data['widget_type'] != $field['widget']['type'])) {
//            $forms[] = 'content_field_basic_form';
//          }
//
//          $forms[] = 'content_field_edit_form';
//          return $forms;
//
//        }
//        else {
//          $field = content_fields($data['field_name']);
//          // add existing field
//          if ($field) {
//            if (empty($data['widget_type'])) {
//               $data['widget_type'] = $field['widget']['type'];
//            }
//
//            $data['type'] = $field['type'];
//
//            if (empty($data['label'])) {
//               $data['label'] = $field['widget']['label'];
//            }
//
//            $data['_add_existing_field'] = array(
//              'label' => $data['label'],
//              'field_name' => $data['field_name'],
//              'widget_type' => $data['widget_type'],
//              'hidden_name' => '_add_existing_field'
//            );
//            if (!empty($data['weight'])) {
//              $data['_add_existing_field']['weight'] = $data['weight'];
//            }
//            if (!empty($data['group'])) {
//              $data['_add_existing_field']['parent'] = $data['group'];
//            }
//
//            $forms[] = 'content_field_overview_form';
//
//            $fields = array('type_name', 'label', 'field_name', 'type', 'widget_type', '_add_existing_field');
//            $extra_fields = array_diff(array_keys($data), $fields);
//            if (!empty($extra_fields)) {
//              $forms[] = 'content_field_edit_form';
//            }
//
//            return $forms;
//          }
//          // add new field
//          else {
//            $data['_add_new_field'] = array(
//              'label' => $data['label'],
//              'field_name' => $data['field_name'],
//              'type' => $data['type'],
//              'widget_type' => $data['widget_type'],
//              'hidden_name' => '_add_new_field'
//            );
//
//            if (!empty($data['weight'])) {
//              $data['_add_new_field']['weight'] = $data['weight'];
//              unset($data['weight']);
//            }
//
//            if (!empty($data['group'])) {
//              $data['_add_new_field']['parent'] = $data['group'];
//            }
//            $forms[] = 'content_field_overview_form';
//
//            $fields = array('type_name', 'label', 'field_name', 'type', 'widget_type', '_add_new_field');
//            $extra_fields = array_diff(array_keys($data), $fields);
//            if (!empty($extra_fields)) {
//              $forms[] = 'content_field_edit_form';
//            }
//
//            return $forms;
//          }
//        }
//      }
//      elseif ($id == 'display') {
//        return 'display';
//      }
//      elseif ($id == 'group') {
//        cache_clear_all('fieldgroup_data', content_cache_tablename());
//        $groups = fieldgroup_groups($data['type_name'], FALSE, TRUE);
//        $group = $groups[$data['group_name']];
//
//        if ($data['delete'] && $group) {
//          return 'fieldgroup_remove_group';
//        }
//        elseif (!$group && !$data['delete']) {
//
//          $data['_add_new_group'] = array(
//            'label' => $data['label'],
//            'group_name' => $data['group_name'],
//            'hidden_name' => '_add_new_group'
//          );
//
//          if (!empty($data['weight'])) {
//            $data['_add_new_group']['weight'] = $data['weight'];
//          }
//          if (!empty($data['group_type'])) {
//            $data['_add_new_group']['group_type'] = $data['group_type'];
//          }
//          if (!empty($data['group_option'])) {
//            $data['_add_new_group']['group_option'] = $data['group_option'];
//            unset($data['group_option']);
//          }
//
//          $forms[] = 'content_field_overview_form';
//
//          if (!empty($data['settings'])) {
//            $forms[] = 'fieldgroup_group_edit_form';
//          }
//
//          return $forms;
//        }
//        elseif ($group) {
//          return 'fieldgroup_group_edit_form';
//        }
//      }
//    break;
//
//    // Add default values to the pattern where appropriate and return form data
//    case 'build':
//      content_clear_type_cache();
//      module_load_include('inc', 'content', 'includes/content.admin');
//
//      if ($id == 'node_type_form') {
//
//        module_load_include('inc', 'node', 'content_types');
//
//        $type = $data['type'];
//        $name = $data['name'];
//        $types = content_types($type);
//
//        if (!$types['type']) {
//          // Set the name of the new content type if missing
//          if (!$data['name']) {
//            $data['name'] = $type;
//          }
//
//          $defaults = _node_type_set_defaults($data);
//          $data = array_merge($defaults, $data);
//        }
//
//        if (empty($data['node_options'])) {
//          $node_options = array('status', 'promote', 'sticky', 'revision');
//          $default_options = variable_get('node_options_'. $type, null);
//
//          $options = array();
//          if (!empty($default_options)) {
//            $options = array_combine($default_options, $default_options);
//          }
//  
//          foreach($data as $key => $value) {
//            if (in_array($key, $node_options)) {
//              if (!empty($value)) {
//                $options[$key] = $key;
//              }
//              else {
//                unset($options[$key]);    
//              }
//              unset($data[$key]);
//            }
//          }
//          $data['node_options'] = array_values($options);
//          $data['node_options']['overwrite'] = TRUE;
//        }
//
//        if (isset($data['has_body']) && !$data['has_body']) {
//          $data['body_label'] = '';
//        }
//      }
//      elseif ($id == 'node_type_delete_confirm') {
//        module_load_include('inc', 'node', 'content_types');
//      }
//      elseif ($id == 'display') {
//
//        if ($data['is_group']) {
//          $groups = fieldgroup_groups($data['type'], FALSE, TRUE);
//          $group = $groups[$data['field']];
//          if ($group) {
//            $group['settings']['display'] = $data[$data['field']] + $group['settings']['display'];
//            fieldgroup_save_group($data['type'], $group);
//          }
//        }
//        else {
//          module_load_include('inc', 'content', 'includes/content.crud');
//          $field = content_fields($data['field'], $data['type']);
//          if ($field) {
//            $field['display_settings'] = $data[$data['field']] + $field['display_settings'];
//            content_field_instance_update($field);
//          }
//        }
//        return t('Your settings have been saved.');
//      }
//      elseif ($id == 'content_field_overview_form') {
//        // due to all the data being passed to multiple forms we may
//        // end up with some CCK field's "title" setting overwriting title
//        // field on this form with some bogus value
//        if (!is_array($data['title'])) {
//          unset($data['title']);
//        }
//      }
//      return $data;
//    break;
//
//    // Create extra parameters needed for a pattern execution
//    case 'params':
//      if ($id == 'node_type_delete_confirm') {
//        $type = (object)content_types($data['type']);
//        return array($type);
//      }
//      elseif ($id == 'node_type_form') {
//        $type = (object)content_types($data['type']);
//        return array($type);
//      }
//      elseif ($id == 'content_field_edit_form' || $id == 'content_field_remove_form') {
//        return array($data['type_name'], $data['field_name']);
//      }
//      elseif ($id == 'content_field_basic_form') {
//        return array($data);
//      }
//      elseif ($id == 'content_field_overview_form') {
//        return $data['type_name'];
//      }
//      elseif ($id == 'fieldgroup_group_edit_form' || $id == 'fieldgroup_remove_group') {
//        return array($data['type_name'], $data['group_name']);
//      }
//    break;
//
//    // Any last cleanup items
//    case 'cleanup':
//      // Make sure content info is always up-to-date
//      cache_clear_all('content_type_info', content_cache_tablename());
//      _content_type_info(true);
//
//      // make sure fieldgroup info is up to date
//      cache_clear_all('fieldgroup_data', content_cache_tablename());
//      if (module_exists('fieldgroup')) {
//        fieldgroup_groups('', FALSE, TRUE);
//      }
//      // filter out confusing warning about 'allowed values' produced by optionwidgets module
//      if ($id == 'content_field_edit_form' && strpos($data['widget_type'], 'optionwidget') === 0 && $warnings = drupal_get_messages('warning')) {
//        foreach ($warnings['warning'] as $warning) {
//          if ($warning != "You need to specify the 'allowed values' for this field.") {
//            drupal_set_message($warning, 'warning');
//          }
//        }
//      }
//    break;
//  }
  
}
