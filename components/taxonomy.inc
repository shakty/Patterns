<?php

//@TODO: handle the case of multiple vocabulary with the same machine name

function taxonomy_patterns($op, $id = NULL, &$data = NULL) {
  switch ($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('vocabulary', 'term', 'terms');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'taxonomy_form_vocabulary' => t('Taxonomy: Add or edit vocabulary'),
        'taxonomy_form_term' => t('Taxonomy: Add or edit a term to a vocabulary'),
        'taxonomy_vocabulary_confirm_delete' => t('Taxonomy: delete a vocabulary'),
      );
    break;

    // Return a summary of an action
    case 'summary':
      switch ($id) {
        case 'taxonomy_form_vocabulary':
          if (!$data['name'] && $data['vid']) {
            $data['name'] = db_query('SELECT name FROM {taxonomy_vocabulary} WHERE vid = :vid', array('vid' => $data['vid']))->fetchField();
          }

          if (!taxonomy_vocabulary_machine_name_load($data['name'])) {
            return t('Create vocabulary %vocab', array('%vocab' => $data['name']));
          }
          else {
            return t('Edit vocabulary %vocab', array('%vocab' => $data['name']));
          }
        break;
        
      }
    break;

    // Prepare data for processing
    case 'prepare':
      
      if ($id == 'vocabulary') {
          if (isset($data['delete'])) {
            $data = _taxonomy_patterns_prepare_vocabulary_delete($data);
          } 
          else {      
            $data = _taxonomy_patterns_prepare_vocabulary($data);
          }
      }
      else if ($id == 'term') {
        $data = _taxonomy_patterns_prepare_term($data);
      }
      else if ($id == 'terms') {       
        // Get the vid first, so that we do not re-request always  
        if (isset($data['vid'])) {
          $vid = $data['vid'];
        }
        else if (isset($data['vocabulary'])) {
          $taxo = taxonomy_vocabulary_machine_name_load($data['vocabulary']);
          $vid = $taxo->vid;
        }
        
        if (isset($vid)) {
          foreach ($data as $term) {
            $terms[] = _taxonomy_patterns_prepare_term($data,$vid);
          }
        }
        
      } // End vocabulary,term,terms
      
    break;

    // Pre validate actions
    // Need to check for required tags etc...
    case 'pre-validate':
      
      if ($id = 'vocabulary') {
        if (!empty($data['vid']) && !taxonomy_vocabulary_load($data['vid'])) {
          return t("Invalid vid: %vid. This vocabulary doesn't exist", array('%vid' => $data['vid']));
        }
      }
      
      
      
    break;

    // Return the form_id('s) for each action
    case 'form_id':
     
        // VOCABULARY
        if ($id =='vocabulary') {
          if (isset($data['delete'])) {
            return 'taxonomy_vocabulary_confirm_delete';
          }
          else {
            return 'taxonomy_form_vocabulary';
          }
        }
        // TERM
        elseif ($id == 'term') {
         if (isset($data['delete'])) {
            return 'taxonomy_term_confirm_delete';
          }
          else {
            return 'taxonomy_form_term';
          }
        }
    break;

    // Prepare for valid processing of this type of component
    case 'build':
      module_load_include('inc', 'taxonomy', 'taxonomy.admin');
      
      return $data;
    break;

    // Validate the values for an action before running the pattern
    case 'validate':
      if ($id == 'taxonomy_form_term') {
        if (!$data['vid'] && !empty($data['vocabulary'])) {
          $vocabs = taxonomy_get_vocabularies();

          foreach ($vocabs as $vid => $vocab) {
            if (strtolower($vocab->name) == strtolower($data['vocabulary'])) {
              $data['vid'] = $vid;
              break;
            }
          }
        }
        // we can't procede without valid vocabulary ID
        if (empty($data['vid'])) {
          return t("Vocabulary %vocab doesn't exist.", array('%vocab' => $data['vocabulary']));
        }
      }
    break;

    // Build a patterns actions and parameters
    case 'params':
      
      if ($id == 'taxonomy_form_vocabulary') {
        if ($data['vid']) {
          return array((array)taxonomy_vocabulary_load($data['vid']));
        }
        else {
          return array(array());
        }
      }
      else if ($id == 'taxonomy_form_term') {
        $vocab = taxonomy_vocabulary_load($data['vid']);
        return array($data, $vocab);
      }
      else if ($id == 'taxonomy_term_confirm_delete') {
        return array($data['tid']);
      }
      
    break;

    // Return term/vocab identifiers
    case 'identifier':
      switch ($id) {
        case 'taxonomy_form_term':
          if (is_numeric($data['tid'])) {
            return $data['tid'];
          }
          else {
            return db_query('SELECT t.tid FROM {taxonomy_term_data} t INNER JOIN {taxonomy_vocabulary} v ON t.vid = v.vid WHERE LOWER(t.name) = LOWER(:name) AND LOWER(v.name) = LOWER(:voc)', array('name' => $data['name'], 'voc' => $data['vocabulary']))->fetchField();
          }
        break;
        case 'taxonomy_form_vocabulary':
          if (is_numeric($data['vid'])) {
            return $data['vid'];
          }
          else {
            return db_query('SELECT vid FROM {taxonomy_vocabulary} WHERE LOWER(name) = LOWER(:name)', array('name' => $data['name']))->fetchField();
          }
        break;
      }
    break;

    // Cleanup any global settings after the action runs
    case 'cleanup':
      unset($_POST['op']);
    break;
  }
}

////////////////////////////////////////////////////////
// Helping Functions
////////////////////////////////////////////////////////

/*
 * Does the standard prepararion for a single term given the vocabulary id
 */
function _taxonomy_patterns_prepare_term ($data, $vid = NULL) {
  
  // Get the vid first
  
  if (isset($data['vocabulary'])) {
    $data['vocabulary_machine_name'] = $data['vocabulary'];
    unset($data['vocabulary']);
  }
  
  // Set the vocabulary-id if not alraeady present
  if (!isset($data['vid'])) {
    // use the parameter if not null
    if (!empty($vid)) {
       $data['vid'] = $vid;
    }
    // Search for it
    else {
      $taxo = taxonomy_vocabulary_machine_name_load($data['vocabulary_machine_name']);
      $data['vid'] = $taxo->vid;
    }
   
  }
  
  if (!isset($data['tid'])) {
    if (isset($data['name'])) {
      // TODO: Check this
      $terms = taxonomy_get_term_by_name($data['name']);
      
      // We set it only if there is one and only term with the same name.
      if(count($terms)==1){
        $term = array_pop($terms);
        $data['tid'] = $term->tid;
      }
    }
  }
  //return 'taxonomy_form_term';
  
  
  if(isset($data['delete'])) {
    $data['confirm_delete'] = TRUE;
  }
  else { // If it was not a delete request do a lot more
    
    $default_weight = 0;
    $default_textformat = 'filtered_html';

    if (!isset($data['format'])) {
      $data['format'] = $default_textformat; // was ''
    }
    
    if (!isset($data['weight'])) {
      $data['weight'] = $default_weight;
    }
    
    // @TODO: Check 'tid' => NULL,
  
    if (isset($data['descr'])) {
      $data['description']['value'] = $data['descr'];
      if (isset($data['descr-format'])) {
        $data['description']['format'] = $data['descr-format'];
        unset($data['descr-format']);
      }
      else {
        $data['description']['format'] = $default_textformat;  
      }
      
      unset($data['descr']);
    }
  }// End Not Delete
  
  return $data;
}

function _taxonomy_patterns_prepare_vocabulary ($data) {
  
  $default_weight = 0;
  $default_hierarchy = 0;

  if (!isset($data['weight'])) {
    $data['weight'] = $default_weight;
  }
  
  if (!isset($data['hierarchy'])) {
    $data['hierarchy'] = $default_hierarchy;
  }
    
    return $data;
}

function _taxonomy_patterns_prepare_vocabulary_delete ($data) {
  // Firt use the vid, if we have it
  if (isset($data['vid'])) {
    $taxo = taxonomy_vocabulary_load($data['vid']);
  }
  else if (isset($data['machine_name'])) {
    $taxo = taxonomy_vocabulary_machine_name_load($data['machine_name']);
  }
  
  $data['vid'] = $taxo->vid;
  $data['name'] = $taxo->machine_name;

  $data['type'] = 'vocabulary';
  $data['confirm'] =  1;
  $data['op'] = 'Delete';
  
  return $data;
}

// @TODO: Check This

//function patterns_form_alter(&$form, &$form_state, $form_id) {
//
//  if (user_access('administer patterns') && variable_get('patterns_form_helper', FALSE)) {
//    $form['#after_build'][] = 'patterns_form_helper';
//  }
//
//  if (patterns_executing()) {
//    // Ensure that parent and related dropdowns display all the terms
//    // including those created during current pattern execution.
//    // Without this, those terms would be omitted due to the static
//    // caching within taxonomy_get_tree().
//    $form_ids = array('taxonomy_form_term');
//    if (in_array($form_id, $form_ids)) {
//      $tid = $form['#term']['tid'];
//      $vid = $form['#vocabulary']['vid'];
//      $parent = array_keys(taxonomy_get_parents($tid));
//
//      $children = _patterns_taxonomy_get_tree($vid, $tid);
//      // A term can't be the child of itself, nor of its children.
//      foreach ($children as $child) {
//        $exclude[] = $child->tid;
//      }
//      $exclude[] = $tid;
//
//      $form['advanced']['parent'] = _patterns_taxonomy_term_select(t('Parents'), 'parent', $parent, $vid, t('Parent terms') . '.', 1, '<' . t('root') . '>', $exclude);
//      $form['advanced']['relations'] = _patterns_taxonomy_term_select(t('Related terms'), 'relations', array_keys(taxonomy_get_related($tid)), $vid, NULL, 1, '<' . t('none') . '>', array($tid));
//    }
//
//    // Ensure that parent item dropdown displays all the menu items
//    // including those created during current pattern execution.
//    // Without this, those menu items would be omitted due to the
//    // static caching within menu_tree_all_data().
//    $form_ids = array('menu_edit_item');
//    if (in_array($form_id, $form_ids)) {
//      if (empty($form['menu']['#item'])) {
//        $item = array('link_title' => '', 'mlid' => 0, 'plid' => 0, 'menu_name' => $form_state['values']['menu']['menu_name'], 'weight' => 0, 'link_path' => '', 'options' => array(), 'module' => 'menu', 'expanded' => 0, 'hidden' => 0, 'has_children' => 0);
//      }
//      else {
//        $item = $form['menu']['#item'];
//      }
//      $form['menu']['parent']['#options'] = _patterns_menu_parent_options(menu_get_menus(), $item);
//    }
//
//    // Ensure that taxonomy dropdowns on node edit form display all the terms
//    // including those created during current pattern execution.
//    if (!empty($form['taxonomy']) && isset($form['type']) && isset($form['#node']) && $form['type']['#value'] . '_node_form' == $form_id) {
//      foreach ($form['taxonomy'] as $vid => $v) {
//        if (!is_numeric($vid)) continue;
//        $form['taxonomy'][$vid] = _patterns_taxonomy_form($vid, $form['taxonomy'][$vid]['#default_value'], $form['taxonomy'][$vid]['#description']);
//      }
//    }
//  }
//
//}

