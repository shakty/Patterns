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
        'taxonomy_form_vocabulary' => t('Save/update vocabularies'),
        'taxonomy_vocabulary_confirm_delete' => t('Delete a vocabulary'),
        'taxonomy_form_term' => t('Save/update taxonomy terms'),
        'taxonomy_term_confirm_delete' => t('Delete a taxonomy term')
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
          if (!taxonomy_get_term_by_name($data['name'])) {
            return t('Create category term %cat for vocabulary %vocab', array('%cat' => $data['name'], '%vocab' => $data['vocabulary']));
          }
          else {
            return t('Edit category %cat in vocabular %vocab', array('%cat' => $data['name'], '%vocab' => $data['vocabulary']));
          }
        break;
        case 'taxonomy_term_confirm_delete':
          return t('Delete category %cat from vocabulary %vocab', array('%cat' => $data['name'], '%vocab' => $data['vocabulary']));
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
      else if ($id == 'vocabulary') {
        
      }
    break;
    
    // Pre validate actions
    // Need to check for required tags etc...
    case 'pre-validate':
      
    break;
    
    // Return the form_id('s) for each action
    case 'form_id':
      if ($data['delete'] && $id == 'term') {
        if ($term = taxonomy_get_term($data['tid'])) {
          return 'taxonomy_term_confirm_delete';
        }
        else {
          return;
        }
      }
      else if ($data['delete'] && $id == 'vocabulary') {
        if ($vocab = taxonomy_get_vocabulary($data['vid'])) {
          return 'taxonomy_term_confirm_delete';
        }
        else {
          return;
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
      if ($id == 'taxonomy_term_confirm_delete' || $id == 'taxonomy_term_confirm_delete') {
        $data['confirm'] = 1;
      }
      else if ($id == 'taxonomy_form_term' || $id == 'taxonomy_term_confirm_delete') {
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
          foreach($terms as $term) {
            if ($term->vid == $data['vid']) {
              $data['tid'] = $term->tid;
            }
          }
        }
      }
      else if ($id == 'taxonomy_form_vocabulary' || $id == 'taxonomy_vocabulary_confirm_delete') {
        if (!$data['vid']) {
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
      
    break;
    
    // Build a patterns actions and parameters
    case 'params':
      if ($id == 'taxonomy_vocabulary_confirm_delete') {
        return $data['vid'];
      }
      else if ($id == 'taxonomy_term_confirm_delete') {
        return $data['tid'];
      }
      else if ($id == 'taxonomy_form_vocabulary') {
        return array($data);
      }
      else if ($id == 'taxonomy_form_term') {
        return array($data['vid'], $data);
      }
    break;
    
    // Reverse actions when disabling a pattern
    case 'reverse':
      if (!$data['delete'] && !$data['reset'] && !$data['disable']) {
        $data['delete'] = true;
      }
      else if ($data['disable']) {
        unset($data['disable']);
      }
      else if ($data['reset']) {
        drupal_set_message(t('Unable to reverse <menu> tags that have %reset set.', array('%reset' => t('reset'))));
      }
      else if ($data['enable']) {
        $data['disable'] = true;
        unset($data['enable']);
      }
      else if ($data['delete']) {
        unset($data['delete']);
      }
      return $data;
    break;
    
    // Cleanup any global settings after the action runs
    case 'cleanup':
      unset($_POST['op']);
    break;
  }
}