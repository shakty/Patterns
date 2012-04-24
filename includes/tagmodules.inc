<?php

/**
 * @file
 * Functions related to build and retrieve information from the
 * *tagmodules* and *moduletags* indexes.
 *
 */


/**
 * Builds an associative array of tags and modules (components).
 *
 * Invokes hook_patterns on all modules, and returns an index of
 * available tags and information associated with them (e.g. from which
 * module they are handled).
 *
 * The index is cached, but reloading can be forced.
 *
 * @param array $data (optional) An associative array of data, that the
 *   components can use to build dynamic forms names. Defaults NULL.
 *   The array usually corresponds to an action in a pattern file, e.g.
 *
 *      array('tag'   => 'node',
 *            'type'  => 'article',
 *            'title' => 'Test Article',
 *            'body', => 'lorem ipsum ...',
 *            );
 *
 * @param bool $reset If TRUE, forces rebuilding the index from all
 *   the components. Defaults FALSE
 *
 * @return array $tagmodules Index of tag and modules
 */
function patterns_tagmodules_get_index($data = NULL, $reset = FALSE) {

  $tagmodules = &drupal_static(__FUNCTION__);

  // Index not yet built or forced rebuilt
  if ($reset || empty($tagmodules)) {
    $tagmodules = array();
    // Get a list of tags and their modules.
    foreach (module_implements('patterns') as $module) {
      _patterns_tagmodules_add_module($module, $tagmodules, $data);
    }
  }
  // We may need to rebuild the index just for the specific tag
  elseif (!empty($data)) {
    $module = patterns_tagmodules_find_module($data, $tagmodules);
    _patterns_tagmodules_add_module($module, $tagmodules, $data);
  }

  // All values from all tags
  return $tagmodules;
}

/**
 * Helper function for <patterns_tagmodules_get_index>.
 *
 * Call <hook_patterns> on a given module and merge the results
 * in the tagmodules index.
 *
 * @param mixed $module A string representing a module (component) name
 * @param array $tagmodules An array containing the tagmodules index
 * @param array $data (optional) An associative array of data, that the
 *   components can use to build dynamic forms names. Defaults NULL.
 *
 * @see patterns_tagmodules_get_index()
 * @see _patterns_tagmodules_add_module()
 */
function _patterns_tagmodules_add_module($module, &$tagmodules, $data = NULL) {
  if (empty($tagmodules)) $tagmodules = array();
  if (empty($module)) return;

  $tags = module_invoke($module, 'patterns', $data);
  _patterns_tagmodules_add_module_to_tags($module, $tags);
  $tagmodules = array_merge($tagmodules, $tags);
}

/**
 * Helper function for <_patterns_tagmodules_add_module>
 *
 * Adds the name of the module to each tag returned from <hook_patterns>.
 *
 * @param mixed $module A string representing a module (component) name
 * @param array $tags Array containing the results of the invocation of
 *   <hook_patterns> on a module.
 */
function _patterns_tagmodules_add_module_to_tags($module, &$tags) {
  if (empty($tags)) return;
  foreach ($tags as $tagname => $value) {
    $tags[$tagname]['module'] = $module;
  }
}

/**
 * Filters out information from a tagmodules index and returns
 * the results in a new array.
 *
 * Select information based on the tag (e.g. node, vocabulary, etc.),
 * or on the key (e.g. PATTERNS_CREATE), or on a combination of both.
 *
 * @param array $tagmodules Associative array of tagmodules
 * @param mixed $tag (optional) Forces to return information only
 *   from this tag. Defaults NULL.
 * @param mixed $key (optional) Forces to return information only
 *   from the tags containing this key. Defaults NULL.
 *
 * @return array $tagmodules The filtered tagmodules array
 */
function patterns_tagmodules_filter($tagmodules, $tag = NULL, $key = NULL) {
  if (empty($tagmodules)) return array();

  // Part of tag
  if (!empty($tag) && !empty($key)) {
    return $tagmodules[$tag][$key];
  }
  // Full tag
  if (!empty($tag)) {
    return $tagmodules[$tag];
  }

  // All the values of key=$key from all tags
  if (!empty($key)) {
    $out = array();
    foreach ($tagmodules as $tagname => $value) {
      if (isset($value[$key])) {
        $out[$value['module']][$tagname] = $value[$key];
      }
    }
    return $out;
  }
}

/**
 * Filters out information from a modulestag index and returns
 * the results in a new array.
 *
 * Select information based on the tag (e.g. node, vocabulary, etc.),
 * or on the key (e.g. PATTERNS_CREATE), or on a combination of both.
 *
 * @param array $moduletags Associative array of moduletags
 * @param mixed $module (optional) Forces to return information only
 *   from this module. Defaults NULL.
 * @param mixed $key (optional) Forces to return information only
 *   from the modules containing this key. Defaults NULL.
 *
 * @return array $tagmodules The filtered tagmodules array
 */
function patterns_moduletags_filter($modulestags, $module = NULL, $key = NULL) {
  if (empty($modulestags)) return array();

  // Part of tag
  if (!empty($module) && !empty($key)) {
    return $moduletags[$module][$key];
  }
  // Full tag
  if (!empty($module)) {
    return $moduletags[$module];
  }

  // All the values of key=$key from all tags
  if (!empty($key)) {
    $out = array();
    foreach ($moduletags as $module => $value) {
      if (isset($value[$key])) {
        $out[$module] = $value[$key];
      }
    }
    return $out;
  }
}


/**
 * Builds up an associative array of modules and exposed tags.
 *
 * @param mixed $module
 *   (optional) If set, only information about this specific module is returned.
 *   Defaults NULL.
 * @param mixed $key (optional) If set, only the values for that key
 *   from all modules are returned. E.g. (PATTERNS_CREATE, PATTERNS_MODIFY, etc.)
 *   Defaults NULL.
 *
 * @return array $moduletags The tagmodules index built according to the parameters.
 *
 * @see patterns_tagmodules_get_index()
 */
function patterns_moduletags_get_index($data = NULL, $reset = FALSE) {
  $moduletags = &drupal_static(__FUNCTION__);

  // Index not yet built
  if ($reset || empty($moduletags)) {

    $moduletags = array();
    foreach (module_implements('patterns') as $component) {
      _patterns_moduletags_add_module($moduletags, $component, $data);
      //$tags = module_invoke($component, 'patterns', $data);
      //$moduletags[$component] = $tags;
    }
  }
  // We may need to rebuild the index just for the specific tag
  elseif (!empty($data)) {
    $module = patterns_tagmodules_find_module($data);
    _patterns_moduletags_add_module($moduletags, $module, $data);
  }

  // All values from all tags
  return $moduletags;
}

/**
 * Helper function for <patterns_api_build_moduletags_index>.
 *
 * Call <hook_patterns> on a given module and merge the results
 * in the moduletags index.
 *
 * @param mixed $module A string representing a module (component) name
 * @param array $moduletags An array containing the moduletags index
 * @param array $data (optional) An associative array of data, that the
 *   components can use to build dynamic forms names. Defaults NULL.
 *
 * @see patterns_tagmodules_get_index()
 * @see _patterns_add_module_to_tags()
 */
function _patterns_moduletags_add_module($module, &$moduletags, $data = NULL) {
  if (empty($moduletags)) $moduletags = array();
  if (empty($module)) return;

  $tags = module_invoke($module, 'patterns', $data);
  $moduletags[$module] = $tags;
}

/**
 * Tries to find the module responsible for a given action.
 *
 * @param array $action Array representing a Patterns action.
 *
 * @return mixed|bool The name of the module responsible for the
 *   action or FALSE, it fails to match the action with a module.
 */
function patterns_tagmodules_find_module($action, $tagmodules = NULL) {
  if (isset($action['module']) && !empty($action['module'])) {
    return $action['module'];
  }

  if (isset($action['tag'])) {
    $tagmodules = (is_null($tagmodules)) ? patterns_tagmodules_get_index($action)
                                         : $tagmodules;
    $tag_name = $action['tag'];
    if (isset($tagmodules[$tag_name]['module'])) {
      return $tagmodules[$tag_name]['module'];
    };
  }
  return FALSE;
}
