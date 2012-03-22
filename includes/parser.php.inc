<?php

// Add the PHP parser to Patterns
patterns_parser_add(PATTERNS_FORMAT_PHP, 'phpparser');


/**
 * Loads and parases a YAML pattern from file.
 *
 * @param string $path The path to the pattern file.
 * @param bool $local (optional) TRUE if the pattern 
 *   file is local. Defaults to TRUE.
 *
 * @return array|bool The pattern structure, or FALSE.
 */
function patterns_phpparser_load($path, $local = TRUE) {
  if ($local && !file_exists($path)) {
    return FALSE;
  }

  if (!$php = file_get_contents($path)) {
    return FALSE;
  }

  return patterns_xmlparser_parse($php);
}


/**
 * Parses a string containing a PHP array and returns 
 * the array.
 * 
 * @param array $pattern The pattern string to convert array.
 * 
 */
function patterns_phpparser_parse($pattern = NULL) {
	if (is_null($pattern)) return FALSE;
	if (is_array($pattern)) return $pattern;
  if (!is_string($pattern)) return FALSE;
  	
  $outout = array();
  parse_str($pattern, $output);
  return $output;
}

/**
 * Converts an array representing a pattern into a YAML string.
 *
 * @param array $pattern The pattern to convert into a YAML string	
 */
function patterns_phpparser_dump($pattern = NULL) {
	if (is_null($pattern) || !is_array($pattern)) return FALSE;
	
  return print_r($pattern, TRUE);
}