<?php

function taxonomy_patterns($op, $id = null, &$data = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('vocabulary', 'term');
    break;

    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        'taxonomy_form_vocabulary' => t('Taxonomy: Add or edit vocabulary'),
        'taxonomy_vocabulary_confirm_delete' => t('Taxonomy: Delete vocabulary'),
        'taxonomy_form_term' => t('Taxonomy: Add or edit taxonomy term'),
        'taxonomy_term_confirm_delete' => t('Taxonomy: Delete taxonomy term'),
      );
    break;

    // Return a summary of an action
    case 'summary':
      switch($id) {
        case 'taxonomy_form_vocabulary':
          if (!$data['name'] && $data['vid']) {
            $data['name'] = db_result(db_query('SELECT name FROM {vocabulary} WHERE vid = "%d"', $data['vid']));
          }

          if (!db_result(db_query('SELECT vid FROM {vocabulary} WHERE name = "%s"', $data['name']))) {
            return t('Create vocabulary %vocab', array('%vocab' => $data['name']));
          }
          else {
            return t('Edit vocabulary %vocab', array('%vocab' => $data['name']));
          }
        break;
        case 'taxonomy_vocabulary_confirm_delete':
          return t('Delete vocabulary %vocab', array('%vocab' => $data['name']));
        break;
        case 'taxonomy_form_term':
          if ($data['delete']) {
            return t('Delete category %cat from vocabulary %vocab', array('%cat' => $data['name'], '%vocab' => $data['vocabulary']));
          }
          else if (!taxonomy_get_term_by_name($data['name'])) {
            return t('Create category term %cat for vocabulary %vocab', array('%cat' => $data['name'], '%vocab' => $data['vocabulary']));
          }
          else {
            return t('Edit category %cat in vocabular %vocab', array('%cat' => $data['name'], '%vocab' => $data['vocabulary']));
          }
        break;
      }
    break;

    // Prepare data for processing
    case 'prepare':
      if ($id == 'term') {
        if ($data['value']) {
          $data['name'] = $data['value'];
          unset($data['value']);
        }

        if ($data['vocab']) {
          $data['vocabulary'] = $data['vocab'];
          unset($data['vocab']);
        }

        if ($data['vocabulary'] && is_numeric($data['vocabulary'])) {
          $data['vid'] = $data['vocabulary'];
          unset($data['vocabulary']);
        }
      }
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
      if ($data['delete'] && $id == 'term') {
        if ($term = taxonomy_get_term($data['tid'])) {
          return 'taxonomy_term_confirm_delete';
        }
      }
      else if ($data['delete'] && $id == 'vocabulary') {
        if ($vocab = taxonomy_vocabulary_load($data['vid'])) {
          return 'taxonomy_vocabulary_confirm_delete';
        }
      }
      else if ($id == 'term') {
        return 'taxonomy_form_term';
      }
      else if ($id == 'vocabulary') {
        return 'taxonomy_form_vocabulary';
      }
    break;

    // Prepare for valid processing of this type of component
    case 'build':
      module_load_include('inc', 'taxonomy', 'taxonomy.admin');
      if (($id == 'taxonomy_form_term' || $id == 'taxonomy_vocabulary_confirm_delete') && $data['delete']) {
        $data['confirm'] = 1;
      }
      else if ($id == 'taxonomy_form_term') {
        if ($data['vocabulary'] && !$data['vid']) {
          $vocabs = taxonomy_get_vocabularies();

          foreach($vocabs as $vid => $vocab) {
            if (strtolower($vocab->name) == strtolower($data['vocabulary'])) {
              $data['vid'] = $vid;
              break;
            }
          }
        }

        if ($data['vid'] && !$data['tid'] && ($terms = taxonomy_get_term_by_name($data['name']))) {
          // @TODO make sure this works even when there are multiple terms returned
          foreach($terms as $term) {
            if ($term->vid == $data['vid']) {
              $data['tid'] = $term->tid;
            }
          }
        }

        // <parent> and <relations> should be proccessed in exactly the same way
        $tags = array('parent', 'relations');

        foreach($tags as $tag) {
          if (!empty($data[$tag])) {
            // handle the case of empty <parent> tag first
            if ($data[$tag] == $tag) {
              $data[$tag] = array();
            }
            // for now we support only term names
//             elseif (is_numeric($data[$tag])) {
//               $data[$tag][$data[$tag]] = $data[$tag];
//             }
            elseif (is_string($data[$tag])) {
              $new = array();
              if ($terms = taxonomy_get_term_by_name($data[$tag])) {
                foreach($terms as $term) {
                  if ($term->vid == $data['vid']) {
                    $new[$term->tid] = $term->tid;
                  }
                }
              }
              $data[$tag] = $new;
            }
            elseif (is_array($data[$tag])) {
              $new = array();
              foreach($data[$tag] as $key => $name) {
                // check if TIDs are passed instead of term names
                // this wouldn't work if term name is a number
                // so for now we support only term names
//                 if (is_numeric($name) && taxonomy_get_term($name)) {
//                   $new[$name] = $name;
//                   continue;
//                 }
                if ($terms = taxonomy_get_term_by_name($name)) {
                  foreach($terms as $term) {
                    if ($term->vid == $data['vid']) {
                      $new[$term->tid] = $term->tid;
                    }
                  }
                }
              }
              $data[$tag] = $new;
            }
          }
        }

        if (!empty($data['synonyms']) && is_array($data['synonyms'])) {
          $data['synonyms'] = implode("\n", $data['synonyms']);
        }
      }
      else if ($id == 'taxonomy_form_vocabulary' || $id == 'taxonomy_vocabulary_confirm_delete') {
        if (!$data['vid'] && !empty($data['name'])) {
          $vocabs = taxonomy_get_vocabularies();

          foreach($vocabs as $vid => $vocab) {
            if (strtolower($vocab->name) == strtolower($data['name'])) {
              $data['vid'] = $vid;
              break;
            }
          }
        }
      }
      return $data;
    break;

    // Validate the values for an action before running the pattern
    case 'validate':
      if ($id == 'taxonomy_form_term') {
        if (!$data['vid'] && !empty($data['vocabulary'])) {
          $vocabs = taxonomy_get_vocabularies();

          foreach($vocabs as $vid => $vocab) {
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
      if ($id == 'taxonomy_vocabulary_confirm_delete') {
        return $data['vid'];
      }
      else if ($id == 'taxonomy_form_vocabulary') {
        if ($data['vid']) {
          return array((array)taxonomy_vocabulary_load($data['vid']));
        }
        else {
          return array(array());
        }
      }
      else if ($id == 'taxonomy_form_term') {
        $vocab = taxonomy_vocabulary_load($data['vid']);
        return array($vocab, $data);
      }
      else if ($id == 'taxonomy_term_confirm_delete') {
        return array($data['tid']);
      }
    break;

    // Return term/vocab identifiers
    case 'identifier':
      switch($id) {
        case 'taxonomy_form_term':
          if (is_numeric($data['tid'])) {
            return $data['tid'];
          }
          else {
            return db_result(db_query('SELECT t.tid FROM {term_data} t INNER JOIN {vocabulary} v ON t.vid = v.vid WHERE LOWER(t.name) = LOWER("%s") AND LOWER(v.name) = LOWER("%s")', $data['name'], $data['vocabulary']));
          }
        break;
        case 'taxonomy_form_vocabulary':
          if (is_numeric($data['vid'])) {
            return $data['vid'];
          }
          else {
            return db_result(db_query('SELECT vid FROM {vocabulary} WHERE LOWER(name) = LOWER("%s")', $data['name']));
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
