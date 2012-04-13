<?php
/*
 * @file
 * Retrieves, sets information about Patterns components.
 *
 */

/**
 * Returns a list of uris to components files. 
 * 
 * Wrapper method for patterns_io_load_components($reset, TRUE);
 * 
 * @param bool $reset (optional) If TRUE, always forces reloading
 *   the components from the file system. Defaults FALSE
 *   
 * @see patterns_io_load_components
 */
function patterns_components_list($reset = FALSE) {
	return patterns_io_load_components($reset, TRUE);
}

/**
 * Returns a list of all the directories containing 
 * Patterns components.
 * 
 * @param bool $reset (optional) If TRUE, forces to 
 *   reload the directory index.
 */
function patterns_components_dirs($reset = FALSE) {
	$components_dirs = &drupal_static(__FUNCTION__);
  
  if (isset($components_dirs) && !$reset) {
    return $components_dirs;
  }

	// Get list of directories to scan for components.
  return module_invoke_all('patterns_components');
}
