<?php

/**
 * @file
 * Push public patterns to friend instances.
 */
function patterns_d2d_get_friend($url) {
  $friends = drupaltodrupal_get_friends();
  foreach ($friends as $friend) {
    if ($friend['url'] === $url) {
      return $friend;
    }
  }
  return FALSE;
}
function patterns_d2d_form_outgoing_configure() {
  $form = array();
  $form['address'] = array(
    '#type' => 'textfield',
    '#title' => t('Address'),
    '#description' => t('Address of a central server to which patterns should be published'),
    '#default_value' => variable_get('patterns_d2d_server', PATTERNS_D2D_SERVER),
    '#size' => 40,
    '#maxlength' => DRUPALTODRUPAL_INSTANCE_URL_MAX_LENGTH,
    '#required' => TRUE,
  );
  $form['auto'] = array(
    '#type' => 'checkbox',
    '#default_value' => variable_get('patterns_d2d_auto_publish', FALSE),
    '#title' => t('Automatically publish patterns'),
    '#description' => t('If checked, patterns are automatically published whenever the \'publish\'-button is hit.'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}
function patterns_d2d_form_incoming_configure() {
  $form = array();
  $form['allow'] = array(
    '#type' => 'checkbox',
    '#default_value' => variable_get('patterns_d2d_allow_publish', FALSE),
    '#title' => t('Allow patterns to be published'),
    '#description' => t('If checked, friends with the required permissions can publish patterns on this server, see D2D\'s groups/permissions.'),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  return $form;
}
function patterns_d2d_form_incoming_configure_submit($form, &$form_state) {
  variable_set('patterns_d2d_allow_publish', $form_state['values']['allow'] ? TRUE : FALSE);
  drupal_set_message(t('The changes have been saved.'));
}
function patterns_d2d_form_outgoing_configure_validate($form, &$form_state) {
  if (!drupaltodrupal_check_url($form_state['values']['address'])) {
    form_set_error('address', t('Address must start with \'http://\' or \'https://\' and end with \'/\'.'));
  }
}
function patterns_d2d_form_outgoing_configure_submit($form, &$form_state) {
  if ($form_state['values']['auto']) {
    drupaltodrupal_blindly_add_friend($form_state['values']['address'], 'patterns server', FALSE);
  }
  variable_set('patterns_d2d_auto_publish', $form_state['values']['auto'] ? TRUE : FALSE);
  variable_set('patterns_d2d_server', $form_state['values']['address']);
  drupal_set_message(t('The changes have been saved.'));
}
function patterns_d2d_share_patterns() {
  $build['main'] = array(
    '#title' => t('Publish Settings'),
    '#type' => 'fieldset',
  );
  $build['main'][] = drupal_get_form('patterns_d2d_form_outgoing_configure');
  $build['main'][] = drupal_get_form('patterns_d2d_form_incoming_configure');
  if (variable_get('patterns_d2d_auto_publish', FALSE)) {
    $friend = patterns_d2d_get_friend(variable_get('patterns_d2d_server', PATTERNS_D2D_SERVER));
    if ($friend === FALSE) {
      drupal_set_message(t('Server not listed as friend instance.'), 'error');
    }
    else {
      drupal_set_message(t('Server is listed as friend instance. Click !link to show details about the instance patterns are published to.', array('!link' => l(t('here'), 'admin/drupaltodrupal/instances/' . $friend['id'] . '/details')))); // TODO: proper t()
    }
  }
  return $build;
}
