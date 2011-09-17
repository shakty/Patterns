<?php
/**
 * @file
 * Retrieve, save, and remove patterns from the database.
 */

/**
 * Remove a pattern from the database.
 *
 * @param mixed $pid
 *   The id of the pattern to remove.
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function patterns_db_remove_pattern($pid) {
  $del = db_delete('patterns')
    ->condition('pid', $pid)
    ->execute();

  return ($del == 1) ? TRUE : FALSE;
}

/**
 * Writes the pattern metadata (and the actual pattern) to the database.
 *
 * @param array $pattern
 *   The pattern represented as an array.
 * @param mixed $path
 *   The new path where the file has just been saved or read from.
 * @param mixed $name
 *   The name of the pattern file without the extension.
 * @TODO Check for success.
 */
function patterns_db_save_pattern($pattern, $path, $name) {

  // Try to parse the array in two different formats.
  if (isset($pattern['info'])) {
    $title = $pattern['info']['title'];
    $description = $pattern['info']['description'];
    $author = $pattern['info']['author'];
  }
  else {

    if (isset($pattern['title'])) {
      $title = $pattern['title'];
    }

    if (isset($pattern['description'])) {
      $description = $pattern['description'];
    }

    if (isset($pattern['author'])) {
      $author = $pattern['author'];
    }

  }

  $pattern_data = db_query("SELECT pid, file, updated FROM {patterns} WHERE name = :name", array('name' => $name))->fetchAssoc();

  // Pattern already in the DB, data needs to be updated.
  if (!empty($pattern_data)) {

    $pid = $pattern_data['pid'];
    // $updated = (db_query("SELECT updated FROM {patterns} WHERE pid = :d", array('d' => $pid))->fetchField());
    $updated = $pattern_data['updated'];
    $file = $pattern_data['file'];

    // Change of location.
    if ($path != $file) {
      $query_params = array('file' => $path, 'updated' => time(), 'pid' => $pid);
      db_query("UPDATE {patterns} SET file = :file, updated = :updated WHERE pid = :pid", $query_params);
    }

    if (($new_updated = filemtime($path)) > $updated) {
      $query_params = array('pattern' => serialize($pattern), 'title' => $title, 'file' => $path, 'updated' => $new_updated, 'descr' => $description, 'pid' => $pid);
      db_query("UPDATE {patterns} SET pattern = :pattern, title = :title, file = :file, updated = :updated, description = :descr WHERE pid = :pid", $query_params);
    }
    else {
      $query_params = array('pattern' => serialize($pattern), 'title' => $title, 'file' => $path, 'descr' => $description, 'pid' => $pid);
      db_query("UPDATE {patterns} SET pattern = :pattern, title = :title, file = :file, description = :descr WHERE pid = :pid", $query_params);
    }
  }

  // It is a new Pattern.
  else {
    // Note: time() != REQUEST_TIME since it's a long process.
    $query_params = array('name' => $name, 'pattern' => serialize($pattern), 'title' => $title, 'file' => $path, 'descr' => $description, 'time' => time());
    db_query("INSERT INTO {patterns} (name, status, file, updated, enabled, title, description, pattern) VALUES ( :name, 0, :file, :time, 0, :title, :descr, :pattern)", $query_params);
  }

  return TRUE;
}

/**
 * Loads the pattern from the DB and return a stdClass obj, containing
 * the pattern code as array (unserialized).
 *
 * @param mixed $id Numeric id or alphanumeric name of the pattern.
 * @return stdClass $pattern
 *   The Pattern obj containing the unsereliazed pattern.
 */
function patterns_get_pattern($id) {
  $pattern = FALSE;
  if (is_numeric($id)) {
    $pattern = db_query("SELECT * FROM {patterns} WHERE pid = :pid", array('pid' => $id))->FetchAll();
  }
  elseif (is_string($id)) {
    $pattern = db_query("SELECT * FROM {patterns} WHERE name = :name", array('name' => $id))->FetchAll();
  }

  if (!$pattern) {
    return FALSE;
  }

  // Pattern data is stored in serialized form in the DB.
  $pattern[0]->pattern = unserialize($pattern[0]->pattern);
  return $pattern[0];
}

/**
 * Loads a pattern object from the database based on the given name.
 *
 * @param $name
 */
function patterns_db_load_pattern_from_name($name = NULL) {

  $query = db_select('patterns', 'p');
  $result = $query->condition('name', $name)
                  ->fields('p')
                  ->execute()
                  ->fetchObject();

  if (!empty($result)) {
    return $result;
  }
  else {
    return FALSE;
  }
}
