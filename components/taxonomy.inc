<?php

//@TODO: handle the case of multiple vocabulary with the same machine name


// Return an error string only in case of error, otherwise modify $data

function taxonomy_patterns () {

  $actions['vocabulary'] = array('descr' => t('Create/Modify/Delete Taxonomy Vocabularies'), 
                                 PATTERNS_CREATE => array('taxonomy_form_vocabulary'),
                                 PATTERNS_MODIFY => array('taxonomy_form_vocabulary'),
                                 PATTERNS_DELETE => array('taxonomy_vocabulary_confirm_delete'),
                                 'files'  => array('modules/taxonomy/taxonomy.admin.inc'),
                                );

  $actions['term'] = array('descr'  => t('Create/Modify/Delete Taxonomy Terms'), 
                           PATTERNS_CREATE => array('taxonomy_form_term'),
                           PATTERNS_MODIFY => array('taxonomy_form_term'),
                           PATTERNS_DELETE => array('taxonomy_term_confirm_delete'),
                           'files'  => array('modules/taxonomy/taxonomy.admin.inc'),
                          );

  return $actions;                
  
}

// Return a list of forms/actions this component can handle
function taxonomy_patterns_actions() {
      return array(
        'taxonomy_form_vocabulary' => t('Taxonomy: Add or edit vocabulary'),
        'taxonomy_form_term' => t('Taxonomy: Add or edit a term to a vocabulary'),
        'taxonomy_vocabulary_confirm_delete' => t('Taxonomy: Delete a vocabulary'),
      );
}  

// Prepare data for processing
function taxonomy_patterns_prepare($action, $tag, &$data = NULL) {
    
  // VOCABULARY
  if ($tag == 'vocabulary') {
    
      if ($action == PATTERNS_DELETE ) {
        $data = _taxonomy_patterns_prepare_vocabulary_delete($action, $data);
      } 
      else { // Create or Modify      
        $data = _taxonomy_patterns_prepare_vocabulary($action, $data);
      }
  } 
      
  // TERM
  else if ($tag == 'term') {
    $data = _taxonomy_patterns_prepare_term($action, $data);
  }
  
  // TERMS
  else if ($tag == 'terms') {       
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
  
  return patterns_results();
}  

// Validate the values for an action before running the pattern
function taxonomy_patterns_validate ($action, $tag, &$data = NULL) {
  
    $status = PATTERNS_SUCCESS;
    $msg = '';
    
    if ($tag == 'term') {
      if (!$data['vid'] && !empty($data['vocabulary'])) {
        $vocabs = taxonomy_get_vocabularies();

        foreach ($vocabs as $vid => $vocab) {
          if (strtolower($vocab->name) == strtolower($data['vocabulary'])) {
            $data['vid'] = $vid;
            break;
          }
        }
      }
      
      // we can't proceed without valid vocabulary ID
      if (empty($data['vid'])) {
        $status = PATTERNS_ERR;
        $msg = t("Vocabulary %vocab doesn't exist.", array('%vocab' => $data['vocabulary']));
      }
    }
    
    return patterns_results($status, $msg);
}

// Return which callback functions to actually use.
function taxonomy_patterns_callbacks ($action, $tag, &$data = NULL) {
	// Just return the form ids specified above. No custom functions used here.
	$desc = taxonomy_patterns();
	$result = $desc[$tag][$action];

	return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Prepare for valid processing of this type of component
function taxonomy_patterns_build ($action, $form_id, &$data = NULL, &$a) {
  module_load_include('inc', 'taxonomy', 'taxonomy.admin'); // TODO: Isn't this already loaded?
  return patterns_results();
}

// Build a patterns actions and parameters
function taxonomy_patterns_params($action, $form_id, &$data = NULL, &$a) {
      
  if ($form_id == 'taxonomy_form_vocabulary') {
    if ($data['vid']) {
      $result = taxonomy_vocabulary_load($data['vid']);
    }
  }
  else if ( $form_id == 'taxonomy_vocabulary_confirm_delete') {
    if ($data['vid']) {
      $result = $data['vid'];
    }
    else if (!isset($data['machine_name'])) {
      $taxo = taxonomy_vocabulary_machine_name_load($data['machine_name']);
      $vid = $taxo->vid;
      if (!empty($vid)) {
        taxonomy_vocabulary_load($data['vid']);
      }
    }
  }
  else if ( $form_id == 'taxonomy_form_term') {
    $vocab = taxonomy_vocabulary_load($data['vid']);
    $result = array($data, $vocab);
  }
  else if ( $form_id == 'taxonomy_term_confirm_delete') {
    $result = $data['tid'];
  }
      
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Cleanup any global settings after the action runs
function taxonomy_patterns_cleanup($action, $tag, &$data = NULL) {
    unset($_POST['op']);
    return patterns_results();
}




////////////////////////////////////////////////////////
// Helping Functions
////////////////////////////////////////////////////////

/*
 * Does the standard prepararion for a single term given the vocabulary id
 */
function _taxonomy_patterns_prepare_term ($action, $data, $vid = NULL) {
  
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
  
  
  if(isset($data['delete'])) { // TODO: action === 'delete'
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

function _taxonomy_patterns_prepare_vocabulary ($action, $data) {
  
  if ($action == PATTERNS_MODIFY) {
    if (!isset($data['vid'])){
      
      // We are modifying the machine_name as well
      if (isset($data['old_machine_name'])){  
        $taxo = taxonomy_vocabulary_machine_name_load($data['old_machine_name']);
        $vid = $taxo->vid;
      }
      // We are changing other stuff
      else if (isset($data['machine_name'])){  
        $taxo = taxonomy_vocabulary_machine_name_load($data['machine_name']);
        if (!empty($taxo)) {
          $vid = $taxo->vid;
          // The old_machine_name must be set equal to machine_name
          $data['old_machine_name'] = $data['machine_name'];
        }
      }
     
      if(!empty($vid)) {
        $data['vid'] = $vid;
      }
      
    } // End: vid missing
  } // End: PATTERNS_MODIFY
  
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

function _taxonomy_patterns_prepare_vocabulary_delete ($action, $data) {
  // Firt use the vid, if we have it
  if (isset($data['vid'])) {
    $taxo = taxonomy_vocabulary_load($data['vid']);
  }
  else if (isset($data['machine_name'])) {
    $taxo = taxonomy_vocabulary_machine_name_load($data['machine_name']);
  }
  
  if (!empty($taxo)){
    $data['vid'] = $taxo->vid;
    $data['name'] = $taxo->machine_name;

    $data['type'] = 'vocabulary';
    $data['confirm'] =  1;
    $data['op'] = 'Delete';
  }
  
  return $data;
}

// Return a summary of an action
//function taxonomy_patterns_summary ($id = NULL, &$data = NULL) {
//  
//  switch ($id) {
//    case 'taxonomy_form_vocabulary':
//      if (!$data['name'] && $data['vid']) {
//        $data['name'] = db_query('SELECT name FROM {taxonomy_vocabulary} WHERE vid = :vid', array('vid' => $data['vid']))->fetchField();
//      }
//
//      if (!taxonomy_vocabulary_machine_name_load($data['name'])) {
//        return t('Create vocabulary %vocab', array('%vocab' => $data['name']));
//      }
//      else {
//        return t('Edit vocabulary %vocab', array('%vocab' => $data['name']));
//      }
//    break;
//  }
//}


// Pre validate actions
// Need to check for required tags etc...
//function taxonomy_patterns_validate($action, $tag, &$data = NULL) {
//      
//    if ($tag = 'vocabulary') {
//      if (!empty($data['vid']) && !taxonomy_vocabulary_load($data['vid'])) {
//        return t("Invalid vid: %vid. This vocabulary doesn't exist", array('%vid' => $data['vid']));
//      }
//    }     
//}

// Return the form_id('s) for each action
//function taxonomy_patterns_form_id ($id = NULL, &$data = NULL) {
//     
//  // VOCABULARY
//  if ($id =='vocabulary') {
//    if (isset($data['delete'])) {
//      return 'taxonomy_vocabulary_confirm_delete';
//    }
//    else {
//      return 'taxonomy_form_vocabulary';
//    }
//  }
//  // TERM
//  elseif ($id == 'term') {
//   if (isset($data['delete'])) {
//      return 'taxonomy_term_confirm_delete';
//    }
//    else {
//      return 'taxonomy_form_term';
//    }
//  }
//}

//    // Return term/vocab identifiers
//    case 'identifier':
//      switch ($id) {
//        case 'taxonomy_form_term':
//          if (is_numeric($data['tid'])) {
//            return $data['tid'];
//          }
//          else {
//            return db_query('SELECT t.tid FROM {taxonomy_term_data} t INNER JOIN {taxonomy_vocabulary} v ON t.vid = v.vid WHERE LOWER(t.name) = LOWER(:name) AND LOWER(v.name) = LOWER(:voc)', array('name' => $data['name'], 'voc' => $data['vocabulary']))->fetchField();
//          }
//        break;
//        case 'taxonomy_form_vocabulary':
//          if (is_numeric($data['vid'])) {
//            return $data['vid'];
//          }
//          else {
//            return db_query('SELECT vid FROM {taxonomy_vocabulary} WHERE LOWER(name) = LOWER(:name)', array('name' => $data['name']))->fetchField();
//          }
//        break;
//      }
//    break;


///////////////////////////////////////////////////////////////////////////
// @TODO: Check This
// FROM NOW ON THIS WAS IN pattern.module, but it is all for taxonomy.inc
//////////////////////////////////////////////////////////////////////////

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


/**
 * Custom implementation of Drupal's _taxonomy_term_select()
 *
 * Used to override static caching in taxonmy_get_tree()
 * which is preventing terms added during patterns execution
 * to be included in parents and relations dropdowns and
 * causes validation errors.
 * Hopefully, we can find better solution for this.
 */
//function _patterns_taxonomy_term_select($title, $name, $value, $vocabulary_id, $description, $multiple, $blank, $exclude = array()) {
//  $tree = _patterns_taxonomy_get_tree($vocabulary_id);
//
//  $options = array();
//
//  if ($blank) {
//    $options[''] = $blank;
//  }
//  if ($tree) {
//    foreach ($tree as $term) {
//      if (!in_array($term->tid, $exclude)) {
//        $choice = new stdClass();
//        $choice->option = array($term->tid => str_repeat('-', $term->depth) . $term->name);
//        $options[] = $choice;
//      }
//    }
//  }
//
//  return array('#type' => 'select',
//    '#title' => $title,
//    '#default_value' => $value,
//    '#options' => $options,
//    '#description' => $description,
//    '#multiple' => $multiple,
//    '#size' => $multiple ? min(9, count($options)) : 0,
//    '#weight' => -15,
//    '#theme' => 'taxonomy_term_select',
//  );
//}

/**
 * Custom implementation of Drupal's taxonomy_get_tree()
 *
 * Removed static caching.
 * New terms may be created during patterns execution and
 * static caching prevents them from being returned in
 * all subsequent calls to taxonomy_get_tree() during
 * the current pattern execution (within current page request)
 */
//function _patterns_taxonomy_get_tree($vid, $parent = 0, $depth = -1, $max_depth = NULL) {
//
//  $depth++;
//
//  $children[$vid] = array();
//
//  // Ste: was
//  //$result = db_query(db_rewrite_sql('SELECT t.tid, t.*, parent FROM {term_data} t INNER JOIN {term_hierarchy} h ON t.tid = h.tid WHERE t.vid = %d ORDER BY weight, name', 't', 'tid'), $vid);
//  $result = db_query(db_rewrite_sql('SELECT t.tid, t.*, parent FROM {taxonomy_term_data} t INNER JOIN {taxonomy_term_hierarchy} h ON t.tid = h.tid WHERE t.vid = :vid ORDER BY weight, name', 't', 'tid'), array('vid' => $vid));
//
//  //Ste: was
//  //while ($term = db_fetch_object($result)) {
//  foreach ($result as $term) {
//    $children[$vid][$term->parent][] = $term->tid;
//    $parents[$vid][$term->tid][] = $term->parent;
//    $terms[$vid][$term->tid] = $term;
//  }
//
//  $max_depth = (is_null($max_depth)) ? count($children[$vid]) : $max_depth;
//  $tree = array();
//  if (!empty($children[$vid][$parent])) {
//    foreach ($children[$vid][$parent] as $child) {
//      if ($max_depth > $depth) {
//        $term = clone $terms[$vid][$child];
//        $term->depth = $depth;
//        // The "parent" attribute is not useful, as it would show one parent only.
//        unset($term->parent);
//        $term->parents = $parents[$vid][$child];
//        $tree[] = $term;
//
//        if (!empty($children[$vid][$child])) {
//          $tree = array_merge($tree, _patterns_taxonomy_get_tree($vid, $child, $depth, $max_depth));
//        }
//      }
//    }
//  }
//
//  return $tree;
//}

/**
 * Custom implementation of Drupal's taxonomy_form()
 *
 * Used to override static caching for taxonomy terms on node edit form.
 */
//function _patterns_taxonomy_form($vid, $value = 0, $help = NULL, $name = 'taxonomy') {
//  $vocabulary = taxonomy_vocabulary_load($vid);
//  $help = ($help) ? $help : $vocabulary->help;
//
//  if (!$vocabulary->multiple) {
//    $blank = ($vocabulary->required) ? t('- Please choose -') : t('- None selected -');
//  }
//  else {
//    $blank = ($vocabulary->required) ? 0 : t('- None -');
//  }
//
//  return _patterns_taxonomy_term_select(check_plain($vocabulary->name), $name, $value, $vid, $help, intval($vocabulary->multiple), $blank);
//}

/**
 * Custom implementation of Drupal's menu_parent_options()
 *
 * Used to override static caching in menu_tree_all_data()
 * which is preventing menu items created during patterns execution
 * to be included in 'parent item' dropdown and causes validation errors.
 * Hopefully, we can find better solution for this.
 */
//function _patterns_menu_parent_options($menus, $item) {
//  // The menu_links table can be practically any size and we need a way to
//  // allow contrib modules to provide more scalable pattern choosers.
//  // hook_form_alter is too late in itself because all the possible parents are
//  // retrieved here, unless menu_override_parent_selector is set to TRUE.
//  if (variable_get('menu_override_parent_selector', FALSE)) {
//    return array();
//  }
//  // If the item has children, there is an added limit to the depth of valid parents.
//  if (isset($item['parent_depth_limit'])) {
//    $limit = $item['parent_depth_limit'];
//  }
//  else {
//    $limit = _menu_parent_depth_limit($item);
//  }
//
//  foreach ($menus as $menu_name => $title) {
//    $tree = _patterns_menu_tree_all_data($menu_name, NULL);
//    $options[$menu_name . ':0'] = '<' . $title . '>';
//    _menu_parents_recurse($tree, $menu_name, '--', $options, $item['mlid'], $limit);
//  }
//  return $options;
//}
//
///**
// * Custom implementation of Drupal's menu_tree_all_data()
// *
// * Removed static caching.
// * New menu items may be created during pattern execution and
// * static caching prevents them from being returned in
// * all subsequent calls to menu_tree_all_data() during
// * the current pattern execution (within current page request)
// */
//function _patterns_menu_tree_all_data($menu_name, $link = NULL, $max_depth = NULL) {
//  $tree = &drupal_static(__FUNCTION__, array());
//
//  // Use $mlid as a flag for whether the data being loaded is for the whole tree.
//  $mlid = isset($link['mlid']) ? $link['mlid'] : 0;
//  // Generate a cache ID (cid) specific for this $menu_name, $link, $language, and depth.
//  $cid = 'links:' . $menu_name . ':all:' . $mlid . ':' . $GLOBALS['language']->language . ':' . (int) $max_depth;
//
//  //if (!isset($tree[$cid])) {
//    // If the static variable doesn't have the data, check {cache_menu}.
//    $cache = cache_get($cid, 'cache_menu');
//    if ($cache && isset($cache->data)) {
//      // If the cache entry exists, it contains the parameters for
//      // menu_build_tree().
//      $tree_parameters = $cache->data;
//    }
//    // If the tree data was not in the cache, build $tree_parameters.
//    if (!isset($tree_parameters)) {
//      $tree_parameters = array(
//        'min_depth' => 1, 
//        'max_depth' => $max_depth,
//      );
//      if ($mlid) {
//        // The tree is for a single item, so we need to match the values in its
//        // p columns and 0 (the top level) with the plid values of other links.
//        $parents = array(0);
//        for ($i = 1; $i < MENU_MAX_DEPTH; $i++) {
//          if (!empty($link["p$i"])) {
//            $parents[] = $link["p$i"];
//          }
//        }
//        $tree_parameters['expanded'] = $parents;
//        $tree_parameters['active_trail'] = $parents;
//        $tree_parameters['active_trail'][] = $mlid;
//      }
//
//      // Cache the tree building parameters using the page-specific cid.
//      cache_set($cid, $tree_parameters, 'cache_menu');
//    }
//
//    // Build the tree using the parameters; the resulting tree will be cached
//    // by _menu_build_tree()).
//    $tree[$cid] = menu_build_tree($menu_name, $tree_parameters);
//  //}
//
//  return $tree[$cid];
//}


/*old drupal6
 * function _patterns_menu_tree_all_data($menu_name = 'navigation', $item = NULL) {
  $tree = array();

  // Use $mlid as a flag for whether the data being loaded is for the whole tree.
  $mlid = isset($item['mlid']) ? $item['mlid'] : 0;
  // Generate a cache ID (cid) specific for this $menu_name and $item.
  $cid = 'links:' . $menu_name . ':all-cid:' . $mlid;

  // If the static variable doesn't have the data, check {cache_menu}.
  $cache = cache_get($cid, 'cache_menu');
  if ($cache && isset($cache->data)) {
    // If the cache entry exists, it will just be the cid for the actual data.
    // This avoids duplication of large amounts of data.
    $cache = cache_get($cache->data, 'cache_menu');
    if ($cache && isset($cache->data)) {
      $data = $cache->data;
    }
  }
  // If the tree data was not in the cache, $data will be NULL.
  if (!isset($data)) {
    // Build and run the query, and build the tree.
    if ($mlid) {
      // The tree is for a single item, so we need to match the values in its
      // p columns and 0 (the top level) with the plid values of other links.
      $args = array(0);
      for ($i = 1; $i < MENU_MAX_DEPTH; $i++) {
        $args[] = $item["p$i"];
      }
      $args = array_unique($args);
      $placeholders = implode(', ', array_fill(0, count($args), '%d'));
      $where = ' AND ml.plid IN (' . $placeholders . ')';
      $parents = $args;
      $parents[] = $item['mlid'];
    }
    else {
      // Get all links in this menu.
      $where = '';
      $args = array();
      $parents = array();
    }
    array_unshift($args, $menu_name);
    // Select the links from the table, and recursively build the tree.  We
    // LEFT JOIN since there is no match in {menu_router} for an external
    // link.
    
    // Ste: was    
//    $data['tree'] = menu_tree_data(db_query("
//      SELECT m.load_functions, m.to_arg_functions, m.access_callback, m.access_arguments, m.page_callback, m.page_arguments, m.title, m.title_callback, m.title_arguments, m.type, m.description, ml.*
//      FROM {menu_links} ml LEFT JOIN {menu_router} m ON m.path = ml.router_path
//      WHERE ml.menu_name = '%s'". $where ."
//      ORDER BY p1 ASC, p2 ASC, p3 ASC, p4 ASC, p5 ASC, p6 ASC, p7 ASC, p8 ASC, p9 ASC", $args), $parents);
    
      $data['tree'] = menu_tree_data(db_query("
      SELECT m.load_functions, m.to_arg_functions, m.access_callback, m.access_arguments, m.page_callback, m.page_arguments, m.title, m.title_callback, m.title_arguments, m.type, m.description, ml.*
      FROM {menu_links} ml LEFT JOIN {menu_router} m ON m.path = ml.router_path
      WHERE ml.menu_name = :args" . $where . "
      ORDER BY p1 ASC, p2 ASC, p3 ASC, p4 ASC, p5 ASC, p6 ASC, p7 ASC, p8 ASC, p9 ASC", array('args' => $args)), $parents);
       
    
    $data['node_links'] = array();
    menu_tree_collect_node_links($data['tree'], $data['node_links']);
    // Cache the data, if it is not already in the cache.
    $tree_cid = _menu_tree_cid($menu_name, $data);
    if (!cache_get($tree_cid, 'cache_menu')) {
      cache_set($tree_cid, $data, 'cache_menu');
    }
    // Cache the cid of the (shared) data using the menu and item-specific cid.
    cache_set($cid, $tree_cid, 'cache_menu');
  }
  // Check access for the current user to each item in the tree.
  menu_tree_check_access($data['tree'], $data['node_links']);
  $tree[$cid] = $data['tree'];

  return $tree[$cid];
}*/
