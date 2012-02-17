<?php
/**
 * @file
 * Retrieve, save, and remove patterns from the database.
 */


/**
 * Mark that pattern status as trashed in the database.
 *
 * @param mixed $pid
 *   The id of the pattern to remove.
 * @return bool
 *   TRUE on success, FALSE otherwise.
 */
function patterns_db_trash_pattern($pid) {
  $trashed = db_update('patterns')
    ->fields(array('status' => PATTERNS_STATUS_REMOVED))
    ->condition('pid', $pid)
    ->execute();

  return ($trashed == 1) ? TRUE : FALSE;
}

/**
 * Remove permanently a pattern from the database.
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

  // TODO: title, description, etc. are actually mandatory fields
  //       in the database, so handle them accordingly.
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

  // Check if it is valid pattern
  if (patterns_validate_pattern($pattern)) {
    $status = PATTERNS_STATUS_OK;
  }
  else {
    $status = PATTERNS_STATUS_INVALID;
  }

  $pattern_data = db_select('patterns', 'p')
                    ->fields('p', array('pid', 'file', 'updated', 'status'))
                    ->condition('p.name', $name)
                    ->execute()
                    ->fetchAssoc();

  // Pattern already in the DB, data needs to be updated.
  if (!empty($pattern_data)) {

    // TODO: how to update the status correctly? Improve!
    if ($pattern_data['status'] == PATTERNS_STATUS_REMOVED) {
      $status = PATTERNS_STATUS_REMOVED;
    }
    else if ($pattern_data['status'] == PATTERNS_STATUS_ENABLED) {
      if ($status == PATTERNS_STATUS_OK) {
        $status = PATTERNS_STATUS_ENABLED;
      }
    }
    
    $pid = $pattern_data['pid'];
    $updated = $pattern_data['updated'];
    $file = $pattern_data['file'];
    
    // Change of location.
    if ($path != $file) {
      $query_params = array('file' => $path, 'updated' => time(), 'pid' => $pid);
      db_query("UPDATE {patterns} SET file = :file, updated = :updated WHERE pid = :pid", $query_params);
    }

    if (($new_updated = filemtime($path)) > $updated) {
      $query_params = array('pattern' => serialize($pattern),
                            'title' => $title,
                            'file' => $path,
                            'updated' => $new_updated,
                            'descr' => $description,
                            'pid' => $pid,
                            'status' => $status,
      );
      db_query("UPDATE {patterns} SET pattern = :pattern, title = :title, file = :file, updated = :updated, status = :status, description = :descr WHERE pid = :pid", $query_params);
    }
    else {
      $query_params = array('pattern' => serialize($pattern),
                            'title' => $title, 'file' => $path,
                            'descr' => $description,
                            'pid' => $pid,
                            'status' => $status,
      );
      db_query("UPDATE {patterns} SET pattern = :pattern, title = :title, file = :file, status = :status, description = :descr WHERE pid = :pid", $query_params);
    }
  }

  // It is a new Pattern.
  else {
    // Note: time() != REQUEST_TIME since it's a long process.
    $query_params = array( 'name' => $name,
                           'pattern' => serialize($pattern),
                           'title' => $title,
                           'file' => $path,
                           'descr' => $description,
                           'time' => time(),
                           'status' => $status,
    );
    db_query("INSERT INTO {patterns} (name, status, file, updated, enabled, title, description, pattern) VALUES ( :name, :status, :file, :time, 0, :title, :descr, :pattern)", $query_params);
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

  $query = db_select('patterns', 'p')
             ->fields('p');

  if (is_numeric($id)) {
    $query = $query->condition('p.pid', $id);
  }
  elseif (is_string($id)) {
    $query = $query->condition('p.name', $id);
  }

  $pattern = $query->execute()->FetchAll();

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
