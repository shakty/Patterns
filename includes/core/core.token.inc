<?php

/**
 * @file
 * Functions to related to token module.
 * TODO: Check if we still need them
 *
 */


/**
 * Implements hook_token_values().
 *
 * @TODO If these get implementated directly into token.module, this should be removed.
 */
function patterns_token_values($type, $object = NULL, $options = array()) {
  if ($type == 'global') {
    $path = conf_path();
    $tokens['confpath'] = $path;
    return $tokens;
  }
}

/**
 * Array walk callback to replace tokens inside form values.
 */
function _patterns_replace_tokens(&$a, &$b, $identifiers = array()) {
  static $count = 0;

  // Replace IDs with identifiers from the current executing pattern
  if (preg_match('/@([a-zA-Z0-9_]+)@/', $a, $match)) {
    $a = str_replace($match[0], $identifiers[$match[1]], $a);
  }
  if (preg_match('/__([a-zA-Z0-9_]+)__/', $b, $match)) {
    $b = str_replace($match[0], $identifiers[$match[1]], $a);
  }

  // Replace tokens
  // Ste: was
  //$a = token_replace($a, 'global', NULL, '@', '@');
  //$b = token_replace($b, 'global', NULL, '__', '__');\

  //$a = token_replace($a, 'global', NULL, '@', '@');
  //$b = token_replace($b, 'global', NULL, '__', '__');


}

/**
 * Recurse an array and replace with tokens
 * @TODO This is used instead of array_walk_recursive because of some strange
 * issues with token_get_values failing.
 */
function _patterns_recurse_tokens(&$object, $identifiers) {
  foreach ($object as $key => $value) {
    if (is_array($value)) {
      _patterns_recurse_tokens($object[$key], $identifiers);
    }
    elseif (is_scalar($value)) {
      $old_key = $key;
      _patterns_replace_tokens($object[$key], $key, $identifiers);

      // The key was changed, change it
      if ($old_key != $key) {
        $keys = array_keys($object);
        $keys[array_search($old_key, $keys)] = $key;
        $object = array_combine($keys, array_values($object));
      }
    }
  }
}
