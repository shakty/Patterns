<?php
/**
 * @file
 * Collectiion of general purpose functions.
 */


function patterns_utils_is_macro_enabled() {
  $list = module_list();
  return (isset($list["macro"])) ? TRUE : FALSE;
}

/**
 * Checks a pattern identifier (numeric or alphanumeric) and if it not
 * valid displays an error message and redirect the user to the specified page.
 * If the pattern is valid, returns it.
 *
 * If no page is specified redirects to '/admin/patterns/'.
 *
 * @param mixed $pattern the pattern identifier
 * @param mixed $back the page to redirect to (default '/admin/patterns/')
 * @param array $options an associative array of extra options. E.g.
 *  $options('numeric' => TRUE) enforces that the pattern id must be numeric.
 *
 *
 */
function patterns_utils_if_invalid_go_back( $pattern = NULL,
                                            $back = '/admin/patterns/',
                                            $options = array()) {

  $id = $pattern; // store a reference to the original parameter
  if (isset($options['numeric'])) {
    if (!is_numeric($pattern)) {
      drupal_set_message(t('You must specify a valid id for the pattern.'));
      return FALSE;
    }
  }

  $pattern = patterns_get_pattern($pattern);

  if (!$pattern) {
    drupal_set_message(t('No pattern was found with the given id: %id', array('%id' => $id)), 'error');
    drupal_goto($back);
  }

  return $pattern;
}

/**
 * Helper function to get PHPDoc @version tag from a file
 */
function patterns_utils_get_phpdoc_version($path) {
  $version = 'unknown';
  $needle  = '@version ';

  if (file_exists($path)) {
    $fp = @fopen($path, 'r');
    if ($fp) {
      while (!feof($fp)) {
        $occurence = stristr(fgets($fp), $needle);
        if ($occurence) { // FALSE if stristr found nothing
          return rtrim(substr($occurence, strlen($needle)));
        }
      }
    }
  }
  return $version;
}



/**
 * Returns a string representation of a pattern object or array.
 * If an id or the name of the pattern is passed, it tries
 * to load it, and then constructs the name of the pattern.
 *
 * @param mixed $pattern
 */
function patterns_utils_toString($pattern = NULL) {
  $pattern = _patterns_db_get_pattern($pattern);
  if (!$pattern) return FALSE;
  return $pattern->name . ' (' . $pattern->title . ')';
}


/**
 * Generate an array representing a standard info section of a pattern.
 * Default values for empty parameters are passed, e.g. the current user
 * for the author, and author's email and the url of the Drupal web site
 * for the author's web site.
 *
 * @param mixed $category
 * @param mixed $descr
 * @param mixed $v
 * @param mixed $core
 * @param mixed $user
 * @param mixed $base_root
 *
 */
// TODO: t()
function patterns_utils_generate_info_section($title = 'Untitled Pattern',
                                              $category = 'General',
                                              $descr = 'No description',
                                              $v = '1.0',
                                              $core = 'x.y',
                                              $user = NULL,
                                              $base_root = NULL ) {
  if (is_null($user)) global $user;
  if (is_null($base_root)) global $base_root;

  $info = array('info' => array());

  $info['info']['title']          = $title;
  $info['info']['author']         = $user->name;
  $info['info']['category']       = $category;
  $info['info']['description']    = $descr;
  $info['info']['core']           = $core;
  $info['info']['author']         = $user->name;
  $info['info']['author_email']   = $user->mail;
  $info['info']['author_website'] = $base_root;

  return $info;
}

/**
 * Checks if current configuration of php enables open remote files
 * for reading.
 *
 */
function patterns_utils_is_fopenurl_enabled() {
  return ini_get('allow_url_fopen') ? TRUE : FALSE;
}

/**
 * Take as input an array representing the pattern code or the pattern as
 * extracted from the database and looks for the specified key.
 *
 * In the following order returns:
 *
 *  - the $value parameter, if not NULL
 *  - the value of the key if found in the INFO section
 *  - the value of the key if found in the first level of the array
 *  - NULL or $fallback if the key is not found
 *
 *
 * @param mixed $key the key to look for
 * @param array $patterm the array to inspect
 * @param mixed $value a default value for the key
 * @param mixes $fallback to return instead of NULL, if no key is found
 *
 */
function patterns_utils_init_from_pattern($key, $pattern, $value = NULL, $fallback = NULL) {
  if (!is_null($value)) return $value;
  if (isset($pattern['info'])) {
    if (isset($pattern['info'][$key])) {
      return $pattern['info'][$key];
    }
  }
  if (isset($pattern[$key])) {
    return $pattern[$key];
  }
  return $fallback;
}

/**
 * Helper function to find out whether a module is currently installed
 *
 * @param mixed $module String representing the name of the module
 *
 * @return Bool TRUE If the module is currently installed
 */
function patterns_utils_is_module_available($module = NULL) {
  if (is_null($module)) return FALSE;
  $modules = system_rebuild_module_data();
  return (isset($modules[$module])) ? TRUE : FALSE;
}

/**
 * Helper function to find out whether a module is currently enabled
 *
 * @param mixed $module String representing the name of the module
 *
 * @return Bool TRUE If the module is currently enabled
 */
function patterns_utils_is_module_enabled($module = NULL) {
  if (is_null($module)) return FALSE;
  $modules = module_list();
  return isset($modules[$module]) ? TRUE : FALSE;
}

/**
 * Checks whether the public patterns page has been enabled.
 *
 * @return Bool TRUE, if a valid url was specified for the
 *   public patterns page.
 */
function patterns_utils_is_public_page_enabled() {
  return (variable_get('patterns_public_url', '') != '') ? TRUE : FALSE;
}

/**
 * Checks whether the current page is the public patterns page
 *
 * @return Bool TRUE, if the current page is the public patterns page
 */
function patterns_utils_is_public_page() {
  $f = variable_get('site_frontpage', '');
  $p = variable_get(PATTERNS_PUBLIC_URL, '');
  if ($f == $p) {
    return drupal_is_front_page();
  }
  return (request_path() == $p) ? TRUE : FALSE;
}
