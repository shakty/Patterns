<?php
/**
 * @file
 * Patterns feed functions.
 */

/**
 * Prints XML feed of published (public) patterns.
 *
 */
function patterns_feed() {
  global $base_url;

  if (!variable_get('patterns_allow_publish', PATTERNS_ALLOW_PUBLISH_DEFAULT)) {
    return drupal_not_found();
  }

  $items = '';

  $patterns = patterns_io_get_patterns(FALSE, FALSE);
  $patterns = $patterns[PATTERNS_STATUS_OK];
  foreach ($patterns as $pattern) {
    if (!$pattern->public) {
      continue;
    }

    $item = "    <pattern>\n";
    $item .= "      <pid>" . $pattern->pid . "</pid>\n";
    $item .= "      <name>" . $pattern->name . "</name>\n";
    $item .= "      <title>" . $pattern->title . "</title>\n";
    $item .= "      <description>" . $pattern->description . "</description>\n";
    $item .= "      <category>" . $pattern->pattern['info']['category'] . "</category>\n";
    $item .= "      <format>" . $pattern->format . "</format>\n";
    $item .= "      <uri>" . $base_url . '/patterns/' . $pattern->pid . "</uri>\n";
    $item .= "    </pattern>\n";

    $items .= $item;
  }

  $header = "  <info>\n";
  $header .= "    <url>" . $base_url . "</url>\n";
//  $header .= "    <description>". $description ."</description>\n";
  $header .= "  </info>\n";

  $feed = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
  $feed .= "<patterns_feed>\n";
  $feed .= $header;
  $feed .= "  <patterns>\n";
  $feed .= $items;
  $feed .= "  </patterns>\n";
  $feed .= "</patterns_feed>\n";

  drupal_add_http_header('Content-Type', 'text/xml; charset=utf-8');
  print $feed;
  exit;
}


function patterns_feed_rearrange_data($feed) {
  unset($feed['tag']);
  foreach ($feed as $key => $section) {
    $tag = $section['tag'];
    if ($tag == 'pattern') {
      unset($section['tag']);
      if (!isset($section['value'])) {
        foreach ($section as $t) {
          if ($t['tag'] == 'pid') {
            $pid = $t['value'];
            break;
          }
        }
        $result[$pid] = patterns_feed_rearrange_data($section);
      }
      else {
        $result[$tag] = $section['value'];
      }
    }
    else {
      unset($section['tag']);
      if (!isset($section['value'])) {
        $result[$tag] = patterns_feed_rearrange_data($section);
      }
      else {
        $result[$tag] = $section['value'];
      }
    }
  }
  return $result;
}
