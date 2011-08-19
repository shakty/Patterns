<?php

function theme_patterns_form_helper_menu($forms) {
  $output = '<ul class="patterns-form-menu">';

  foreach ($forms as $form_id => $values) {
    $output .= '<li class="patterns-form-menu-item">' . $form_id . '</li>';
  }

  $output .= '</li>';

  return $output;
}

function theme_patterns_form_helper($form_id, $values) {

  $output = '<div class="patterns-form" id="patterns-form-' . $form_id . '">';

  $output .= '<div class="patterns-form-title">' . t('Form values for %key', array('%key' => $form_id)) . '</div>';

  foreach ($values as $key => $value) {
    $output .= '<div class="patterns-form-item"><div class="patterns-form-key">' . $key . ' => </div>';
    $output .= '<div class="patterns-form-value">' . print_r($value, TRUE) . '</div></div>';
  }

  $output .= '</div>';

  return $output;
}

/**
 * Implements hook_theme().
 */
function patterns_theme() {
  
  


  return array(
    'patterns_form_helper' => array(
      'arguments' => array('form_id' => NULL, 'values' => NULL),
    ),
    'patterns_form_helper_menu' => array(
      'forms' => NULL
    ),
    'patterns_list' => array(
      'render element' => 'list',
      //'pattern' => 'patterns__',
    ),
  );
 
}


function theme_patterns_list ($patterns) {
  
  if (empty($patterns)) {
    $output.= t('No patterns available.');
    return $output;
  }
  
  $header = array(t('Title'), t('Status'), t('Version'), t('Actions'));

  // List all patterns
  $rows = array();
  
  foreach ($patterns as $pid => $pattern) {
    $actions = array();
    
    // Build Run/Re-Run/Update pattern links
    if (!$pattern->status) {
      $actions[] =  l(t('Run'), 'admin/patterns/enable/' . $pid);
    }
    elseif ($pattern->enabled >= $pattern->updated) {
      $actions[] =  l(t('Re-Run'), 'admin/patterns/enable/' . $pid);
    }
    else {
      $actions[] = l(t('Run Update'), 'admin/patterns/enable/' . $pid);
    }
    // Edit Link
    $actions[] = l(t('Edit'), 'admin/patterns/edit/' . $pid);
    
    // Remove Link
    $actions[] = l(t('Remove'), 'admin/patterns/remove/' . $pid);
    
    //Publish/Unpublish link
    if (variable_get('patterns_allow_publish', FALSE)) {
      $actions[] = $pattern->public ? l(t('Unpublish'), 'admin/patterns/unpublish/' . $pid) : l(t('Publish'), 'admin/patterns/publish/' . $pid);
    }
    
    $actions = implode('&nbsp;&nbsp;', $actions);

    
    //      $title = l($pattern->title, 'admin/patterns/info/'. $pid, array('attributes' => array('class' => 'pattern-title', 'id' => 'pid-'. $pid)));
    $title = '<span id="pid-' . $pid . '" class="pattern-title">' . $pattern->title . '</span>';
    //     $view_more = '<div>'. t('Clik on pattern title to see more details.') .'</div>';
    $info = array();
    $info[] = t('Author:') . ' ' . @$pattern->info['author'];
    $info[] = t('Email:') . ' ' . @$pattern->info['author_email'];
    $info[] = t('Web:') . ' ' . @$pattern->info['author_website'];
    $author = theme('item_list', array('items' => $info));
    //    $title .= '<div id="pid-'. $pid .'-info" class="pattern-info">'. $author . $pattern->description . $view_more .'</div>';
    $title .= '<div id="pid-' . $pid . '-info" class="pattern-info">' . $author . $pattern->description . '</div>';
    
    $cells = array();
    $cells[] = array('data' => $title, 'class' => 'title');
    $cells[] = array('data' => $pattern->status ? t('Enabled') : t('Disabled'), 'class' => 'status');
    $cells[] = array('data' => @$pattern->info['version'], 'class' => 'version');
    $cells[] = array('data' => $actions, 'class' => 'actions');
    
    // Group patterns by category
    $category = $pattern->info['category'] ? $pattern->info['category'] : t('Other');
    
    $rows[$category][] = $cells;
  }

  // Builds as many fieldsets as pattern categories
  // and plot tables with the rows we created before
  ksort($rows);
  foreach ($rows as $title => $category) {
    $fieldset = array('element' => array(
      '#title' => t($title),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#children' => t($title), 
      '#value' => theme('table', array('rows' => $category, 'attributes' => array('class' => 'patterns-list'))),
    ));
    $output .= theme('fieldset', $fieldset);
  }
  
  return $output;
}
