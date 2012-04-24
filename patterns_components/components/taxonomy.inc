<?php
/**
 * @file
 * Patterns component for taxonomy vocabularies and terms.
 */
//@TODO: handle the case of multiple vocabulary with the same machine name


// Return an error string only in case of error, otherwise modify $data

function taxonomy_patterns($data = NULL) {

  $actions['vocabulary'] = array(
    PATTERNS_INFO   => t('Create/Modify/Delete Taxonomy Vocabularies'),
    PATTERNS_CREATE => array('taxonomy_form_vocabulary'),
    PATTERNS_MODIFY => array('taxonomy_form_vocabulary'),
    PATTERNS_DELETE => array('taxonomy_vocabulary_confirm_delete'),
    PATTERNS_FILES  => array('modules/taxonomy/taxonomy.admin.inc'),
    PATTERNS_EXPORT => array(
                          PATTERNS_EXPORT_ALL => 'taxonomy_patterns_export_all_vocs',
                          // Commented are more options for extraction
                          // This is not yet working
                          // 'Cheat (I am doing terms)' => 'taxonomy_patterns_export_all_terms',
                          //'Single Vocabulary' => array('callback' => 'taxonomy_patterns_export_single_vocabulary',
                          //                             'arguments' => array('1', '2', '3'),
                          //),
                        ),
  );

  $actions['term'] = array(
    PATTERNS_INFO   => t('Create/Modify/Delete Taxonomy Terms'),
    PATTERNS_CREATE => array('taxonomy_form_term'),
    PATTERNS_MODIFY => array('taxonomy_form_term'),
    PATTERNS_DELETE => array('taxonomy_term_confirm_delete'),
    PATTERNS_FILES  => array('modules/taxonomy/taxonomy.admin.inc'),
    PATTERNS_EXPORT => array(
                          PATTERNS_EXPORT_ALL => 'taxonomy_patterns_export_all_terms',
                          //'Single Term' => array('callback' => 'taxonomy_patterns_export_single_vocabulary',
                          //                       'arguments' => 'taxonomy_patterns_get_terms_args',
                          //),
                        ),
  );

  /* Not used anymore

  $actions['vocabulary_extract_all']  = array(
    PATTERNS_INFO => t('Create/Modify/Delete Taxonomy Vocabularies'),
    PATTERNS_MODIFY => array('taxonomy_form_vocabulary'),
    PATTERNS_FILES => array('modules/taxonomy/taxonomy.admin.inc'),
  );

  $actions['term_extract_all'] = array(
    PATTERNS_INFO => t('Create/Modify/Delete Taxonomy Terms'),
    PATTERNS_MODIFY => array('taxonomy_form_term'),
    PATTERNS_FILES => array('modules/taxonomy/taxonomy.admin.inc'),
  );
  */

  return $actions;

}

// Return a list of forms/actions this component can handle
function taxonomy_patterns_actions($data) {
  return array(
    'taxonomy_form_vocabulary' => t('Taxonomy: Add or edit vocabulary'),
    'taxonomy_form_term' => t('Taxonomy: Add or edit a term to a vocabulary'),
    'taxonomy_vocabulary_confirm_delete' => t('Taxonomy: Delete a vocabulary'),
  );
}


function taxonomy_patterns_export_all_vocs($args = NULL, &$result = NULL) {
  $info = taxonomy_patterns();
  $form_id = current($info['vocabulary'][PATTERNS_MODIFY]);
  $vocs = taxonomy_get_vocabularies();
  $result = array();
  foreach ($vocs as $voc) {
    // It is important to user array merge. Pushing is not enough
    $result = array_merge($result, patterns_api_extract_actions($form_id, $voc, 'vocabulary', PATTERNS_MODIFY));
  }
  return $result;
}

function taxonomy_patterns_export_all_terms($args = NULL, $result = NULL) {
  $info = taxonomy_patterns();
  $terms = entity_load('taxonomy_term');
  $form_id = current($info['term'][PATTERNS_MODIFY]);
  $result = array();
  foreach ($terms as $term) {
    // It is important to user array merge. Pushing is not enough
    $result = array_merge($result, patterns_api_extract_actions($form_id, $term, 'term', PATTERNS_MODIFY));
  }
  return $result;
}

// returns arguments needed for the forms when processing automatically
// $loop is set if we want to extract all values
function taxonomy_patterns_get_arguments($action, $tag, $form_id, &$loop = FALSE) {
  static $i = -1;
  $i++;

  if ($tag == 'vocabulary' || $tag == 'vocabulary_extract_all') {
    $vocabulary = taxonomy_get_vocabularies();
    if ($tag == 'vocabulary' && $action == PATTERNS_MODIFY
        && $form_id == 'taxonomy_form_vocabulary') {
          return array(array_pop($vocabulary));
    }
    elseif ($tag == 'vocabulary_extract_all') {
      $loop = TRUE;
      $return = array();
      foreach ($vocabulary as $voc) {
        array_push($return, array($voc));
      }
      return $return;
    }
  }
  elseif ($tag == 'term' || $tag == 'term_extract_all') {
    $terms = entity_load('taxonomy_term');
    if ($tag == 'vocabulary' && $action == PATTERNS_MODIFY
        && $form_id == 'taxonomy_form_vocabulary') {
      return array(array_pop($terms));
    }
    elseif ($tag == 'term_extract_all') {
      $loop = TRUE;
      $return = array();
      foreach ($terms as $term) {
        array_push($return, array($term));
      }
      return $return;
    }
  }

}

// Prepare data for processing
function taxonomy_patterns_prepare($action, $tag, &$data) {
  // dirty fix for the int <-> string problem
  if (isset($data['name']))
    $data['name'] = strval($data['name']);
  if (isset($data['machine_name']))
    $data['machine_name'] = '' . $data['machine_name'];
  if (isset($data['old_machine_name']))
    $data['old_machine_name'] = '' . $data['old_machine_name'];

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
  elseif ($tag == 'term') {
    $data = _taxonomy_patterns_prepare_term($action, $data);
  }

  // TERMS
  elseif ($tag == 'terms') {
    // Get the vid first, so that we do not re-request always
    if (isset($data['vid'])) {
      $vid = $data['vid'];
    }
    elseif (isset($data['vocabulary'])) {
      $taxo = taxonomy_vocabulary_machine_name_load($data['vocabulary']);
      $vid = $taxo->vid;
    }

    if (isset($vid)) {
      foreach ($data as $term) {
        $terms[] = _taxonomy_patterns_prepare_term($data, $vid);
      }
    }

  } // End vocabulary,term,terms

  return patterns_results();
}

// Validate the values for an action before running the pattern
function taxonomy_patterns_validate($action, $tag, &$data) {

  $status = PATTERNS_SUCCESS;
  $msg = '';

  if ($tag == 'term') {
    if (!$data['vid'] && !empty($data['vocabulary'])) {
      $vocabs = taxonomy_get_vocabularies();

      foreach ($vocabs as $vid => $vocab) {
        if (drupal_strtolower($vocab->name) == drupal_strtolower($data['vocabulary'])) {
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
function taxonomy_patterns_callbacks($action, $tag, &$data) {
  // Just return the form ids specified above. No custom functions used here.
  $desc = taxonomy_patterns();
  $result = $desc[$tag][$action];
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Prepare for valid processing of this type of component
function taxonomy_patterns_build($action, $form_id, &$data = NULL, &$a) {
  //module_load_include('inc', 'taxonomy', 'taxonomy.admin'); // TODO: Isn't this already loaded?
  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $data);
}

// Build a patterns actions and parameters
function taxonomy_patterns_params($action, $form_id, &$data = NULL, &$a) {

  if ($form_id == 'taxonomy_form_vocabulary') {
    if ($data['vid']) {
      $result = taxonomy_vocabulary_load($data['vid']);
    }
  }
  elseif ( $form_id == 'taxonomy_vocabulary_confirm_delete') {
    if ($data['vid']) {
      $result = $data['vid'];
    }
    elseif (!isset($data['machine_name'])) {
      $taxo = taxonomy_vocabulary_machine_name_load($data['machine_name']);
      $vid = $taxo->vid;
      if (!empty($vid)) {
        taxonomy_vocabulary_load($data['vid']);
      }
    }
  }
  elseif ( $form_id == 'taxonomy_form_term') {
    $vocab = taxonomy_vocabulary_load($data['vid']);
    $result = array($data, $vocab);
  }
  elseif ( $form_id == 'taxonomy_term_confirm_delete') {
    $result = $data['tid'];
  }

  return patterns_results(PATTERNS_SUCCESS, t('Execution successful'), $result);
}

// Cleanup any global settings after the action runs
function taxonomy_patterns_cleanup($action, $tag, &$data) {
  unset($_POST['op']); // TODO: ???
  return patterns_results();
}




////////////////////////////////////////////////////////
// Helping Functions
////////////////////////////////////////////////////////

/*
 * Does the standard prepararion for a single term given the vocabulary id
 */
function _taxonomy_patterns_prepare_term($action, $data, $vid = NULL) {

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
      if (count($terms) == 1) {
        $term = array_pop($terms);
        $data['tid'] = $term->tid;
      }
    }
  }

  if (isset($data['parent'])) {
    $data['parent'] = _taxonomy_patterns_prepare_parent_terms($data['parent']);
  }


  //return 'taxonomy_form_term';


  if (isset($data['delete'])) { // TODO: action === 'delete'
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
  }
  // End Not Delete

  return $data;
}

/**
 * Checks if the provided array of potential parents is valid, and
 * in case try to correct it.
 *
 *
 * @param $parents mixed The potential array of parents.
 */
function _taxonomy_patterns_prepare_parent_terms($parents) {
  if (!isset($parents)) return FALSE;
  if (!is_array($parents)) {
    $parents = array($parents);
  }
  $err = FALSE;
  $out = array();
  // Terms can have multiple parents
  foreach ($parents as $p) {
    if (!is_numeric($p)) {
      // Try to find out if a term with exists with the given name
      $ps = taxonomy_get_term_by_name($p);
      if (count($ps) !== 1) {
        $err = TRUE;
        continue;
      }
      $parent = array_pop($ps);
    }
    elseif ($p === 0) {
        $parent = new stdClass();
        $parent->tid = 0;
    }
    else {
      $parent = taxonomy_term_load($p);
      if (!$parent) {
        $err = TRUE;
        continue;
      }
    }
    $out[] = $parent->tid;
  }

  if ($err) {
    drupal_set_message(t('One or more error occurred while detecting parent terms.'), 'warning');
  }

  return $out;
}

function _taxonomy_patterns_prepare_vocabulary($action, $data) {

  if ($action == PATTERNS_MODIFY) {
    if (!isset($data['vid'])) {

      // We are modifying the machine_name as well
      if (isset($data['old_machine_name'])) {
        $taxo = taxonomy_vocabulary_machine_name_load($data['old_machine_name']);
        $vid = $taxo->vid;
      }
      // We are changing other stuff
      elseif (isset($data['machine_name'])) {
        $taxo = taxonomy_vocabulary_machine_name_load($data['machine_name']);
        if (!empty($taxo)) {
          $vid = $taxo->vid;
          // The old_machine_name must be set equal to machine_name
          $data['old_machine_name'] = $data['machine_name'];
        }
      }

      if (!empty($vid)) {
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

function _taxonomy_patterns_prepare_vocabulary_delete($action, $data) {
  // Firt use the vid, if we have it
  if (isset($data['vid'])) {
    $taxo = taxonomy_vocabulary_load($data['vid']);
  }
  elseif (isset($data['machine_name'])) {
    $taxo = taxonomy_vocabulary_machine_name_load($data['machine_name']);
  }

  if (!empty($taxo)) {
    $data['vid'] = $taxo->vid;
    $data['name'] = $taxo->machine_name;

    $data['type'] = 'vocabulary';
    $data['confirm'] =  1;
    $data['op'] = 'Delete';
  }

  return $data;
}

