<?php

// Register the YAML parser to Patterns
patterns_parser_add(PATTERNS_FORMAT_YAML, 'yamlparser');


/**
 * Loads and parases a YAML pattern from file.
 *
 * @param string $path The path to the pattern file.
 * @param bool $local (optional) TRUE if the pattern 
 *   file is local. Defaults to TRUE.
 *
 * @return array|bool The pattern structure, or FALSE.
 */
function patterns_yamlparser_load($path, $local = TRUE) {
  if ($local && !file_exists($path)) {
    return FALSE;
  }

  require_once libraries_get_path('spyc') . '/spyc.php';
  return Spyc::YAMLLoad($path);
}


/**
 * Parses a string representing a YAML pattern and return 
 * the array representation.
 * 
 * @param array $pattern The pattern string to convert array.
 * 
 */
function patterns_yamlparser_parse($pattern) {
  if (!is_array($pattern)) {
    require_once libraries_get_path('spyc') . '/spyc.php';
    $pattern = Spyc::YAMLLoad($pattern);
  }
  return $pattern;
}

/**
 * Converts an array representing a pattern into a YAML string.
 *
 * @param array $pattern The pattern to convert into a YAML string	
 */
function patterns_yamlparser_dump($pattern = NULL) {
	if (is_null($pattern) || !is_array($pattern)) return FALSE;
	
  require_once libraries_get_path('spyc') . '/spyc.php';
  $pattern = Spyc::YAMLDump($pattern);
  return $pattern;
}