<?php



/**
 * Creates a pattern from an XML data source.
 *
 * @param string $xml The XML string of the data source.
 *
 * @return array|bool
 *   The pattern structure, or FALSE.
 */
// function patterns_from_source($xml) {
function patterns_parser_xml($xml) {
  $parse = drupal_xml_parser_create($xml);
  xml_parser_set_option($parse, XML_OPTION_CASE_FOLDING, 0);

  if (!xml_parse_into_struct($parse, $xml, $vals, $index)) {
    // TODO: Report parsing or other error.
    return FALSE;
  }

  // Create a multi-dimensional array representing the XML structure.
  $pattern = current(_patterns_parse_tag($vals));

  return $pattern;
}

function patterns_rearrange_data($pattern) {
  foreach ($pattern as $key => $value) {
    if (is_string($key)) {
      unset($pattern[$key]);
    }
    else {
      
     if ($value['xml_tag'] == PATTERNS_SECTION_INFO) {
        $pattern[PATTERNS_SECTION_INFO] = _patterns_rearrange_data($value);
        unset($pattern[PATTERNS_SECTION_INFO]['xml_tag']);
        unset($pattern[$key]); // TODO: unset in _patterns_rearrange_data
     }
     elseif ($value['xml_tag'] == PATTERNS_SECTION_MODULES) {
        // TODO  
     }
     
     // Normal section
     else {
        $section = $value['xml_tag']; 
        $pattern[$section] = array();
        unset($value['xml_tag']);
        foreach($value as $action) { 
          $tag = array('tag' => @$action['tag']); // TODO: check when this does not exist
          $_action = $action['xml_tag']; // TODO: check that is a valid one and that exist
          //$pattern[$section][] = array($_action => array_merge($tag, _patterns_rearrange_data($action)));
          $_action_data = _patterns_rearrange_data($action);
          unset($_action_data['xml_tag']);
          $pattern[$section][] = array($_action => $_action_data);
        }
        unset($pattern[$key]);
      }
     
    }
  }

  return $pattern;
}

function _patterns_rearrange_data($data, $parent = '') {
  $numeric = array();
  $count = 0;
  
  foreach ($data as $key => $value) {
    if (isset($value['value'])) {
      if ($value['value'] == 'false') {
        $value['value'] = FALSE;
      }
      elseif ($value['value'] == 'true') {
        $value['value'] = TRUE;
      }
    }

    if (is_numeric($key) && is_array($value) && count($value) == 2 && isset($value['tag']) && isset($value['value'])) {
      unset($data[$key]);
      if (isset($data[$value['xml_tag']])) {
        $numeric[] = $value['xml_tag'];
        $data[$count++] = $data[$value['xml_tag']];
        $data[$count++] = $value['value'];
        unset($data[$value['xml_tag']]);
      }
      elseif (in_array($value['tag'], $numeric)) {
        $data[$count++] = $value['value'];
      }
      else {
        $data[$value['xml_tag']] = $value['value'];
      }
    }
    elseif (is_numeric($key)) {
      $tag = $value['xml_tag'];
      unset($value['xml_tag']);
      // was 
      //$data[$tag][] = _patterns_rearrange_data($value, $tag);
      $value = _patterns_rearrange_data($value, $tag);
      $data[$tag] = $value['value'];
      unset($data[$key]);
    }
  }

  foreach ($data as $key => $value) {
    if (is_array($value) && count($value) == 1 && $value[0]) {
      $data[$key] = $data[$key][0];
    }
  }

  // This workaround enables us to define numeric keys in XML by
  // prefixing the number with single character. E.g <n0>value</n0>
  // will result in 0 => 'value' (first character of the key will be removed).
  if (isset($data['_numeric_keys'])) {
    unset($data['_numeric_keys']);
    foreach ($data as $key => $value) {
      $data[substr($key, 1)] = $value;
      unset($data[$key]);
    }
  }
  
  return $data;
 
}

/**
 * @file
 * Functions related to parsing pattern files.
 */

/**
 * Finds parts of an array based on a semi-compatible Xpath syntax.
 *
 * Returns an array of constructs that includes the references 'item' and
 * 'parent' from the matching values in the $data object along with extra keys
 * 'key' for the key of the current match and 'trace' for a full list of keys
 * until the root of the $data object.
 *
 * Loosely based off of Cake function Set::extract.
 *
 * @Note: Ensure this always only returns matches from a single level in the
 * array. Changes made to matches in different levels can possibly mess up the
 * above-level matches. To this end, you cannot change the keys of a current
 * match because the other matches cannot change their keys as well to sync with
 * the new parent array.
 * @TODO: Think of a solution for the above note (BROKEN RIGHT NOW).
 */
function patterns_array_fetch($path, &$data, $context = NULL) {
  if (!$context && (empty($data) || !is_array($data))) {
    return array();
  }
  if ($path === '/') {
    return $data;
  }

  // Construct our contexts object that allows us to traverse the array
  if (!$context) {
    $context = _patterns_array_context($data);
  }

  // Make our context actually a list of contexts
  $context = array($context);

  // Create a list of tokens based on the supplied path
  $tokens = array_slice(preg_split('/(?<!=)\/(?![a-z]*\])/', $path), 1);

  while (!empty($tokens)) {
    $token = array_shift($tokens);

    // TODO Implement better conditionals for each token
    // Currently only supports element=value conditions
    $conditions = array();
    if (preg_match('/(=)(.*)/', $token, $m)) {
      $conditions[$m[1]] = $m[2];
      $token = substr($token, 0, strpos($token, $m[1]));
    }

    $matches = array();
    foreach ($context as &$piece) {
      if ($token === '..') {
        $matches[] = &$piece['parent'];
        continue;
      }
      $match = FALSE;
      if (is_array($piece['item']) && ($token == '*' || array_key_exists($token, $piece['item']))) {
        $i = 0;
        while (isset($piece[$i])) {
          if ($piece[$i]['key'] === $token) {
            $matches[] = &$piece[$i];
            break;
          }
          elseif ($token === '*') {
            $matches[] = &$piece[$i];
          }
          $i++;
        }
      }
      elseif ($token === '.') {
        $matches[] = &$piece;
      }
    }

    // Filter matches from the matches list based on our conditions
    foreach ($conditions as $operator => $value) {
      _patterns_array_filter($matches, $operator, $value);
    }

    // Update the context area to the next set of matches to dig into
    $context = $matches;
  }

  // Return the list of matches containing references to their respective data objects
  return $matches;
}

/**
 * Recurse through the values of a parsed xml file to create a
 * multi-dimensional representation of the data.
 */
function _patterns_parse_tag($data, &$index = 0) {
  $pattern = array();

  while (isset($data[$index]) && $current = $data[$index]) {
    $type = $current['type'];

    // Changing tag into xml_tag,
    // otherwise it conflicts with the key 'tag' of the pattern
    $current['xml_tag'] = $current['tag'];
    unset($current['tag']);
     
    if (!empty($current['attributes'])) {
      foreach ((array) $current['attributes'] as $key => $value) {
        $current[$key] = $value;
      }
    }

    unset($current['type'], $current['level'], $current['attributes']);

    if (isset($current['value']) && !trim($current['value']) && $current['value'] != "0") {
      unset($current['value']);
    }

    switch ($type) {
      case 'open':
        $index++;
        $current += _patterns_parse_tag($data, $index);
        $pattern[] = $current;
        break;
      case 'close':
        $index++;
        return $pattern;
        break;
      case 'complete':
        // In order to support more complex/non-standard features we can use serialized data
        if (!empty($current['attributes']['serialized'])) {
          $value = unserialize($current['value']);

          if (isset($value)) {
            $current['value'] = $value;
          }
        }

        // If no value was specified, make sure an empty value is there
        if (!isset($current['value'])) {
          $current['value'] = '';
        }

        $pattern[] = $current;
        break;
    }

    $index++;
  }

  return $pattern;
}


/**
 * Validates an XML file.
 *
 * @TODO Doc.
 * TODO: test it
 * TODO: improve XML validator
 * Check that the xml was properly parsed and also that the
 */
function patterns_parser_validate_xml_pattern($content) {

  if (empty($content)) {
    return FALSE;
  }

  // Add XML tag if missing.
  if (strpos($content, '<?xml') !== 0) {
    $content = '<?xml version="1.0" encoding="UTF-8"?>' . $content;
  }

  $parse = drupal_xml_parser_create($content);
  $success = xml_parse_into_struct($parse, $content, $vals, $index);

  if (!$success || !$vals || $vals[0]['tag'] != 'PATTERN' || $vals[1]['tag'] != 'INFO') {
    return FALSE;
  }

  return TRUE;
}



