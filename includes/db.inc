<?php
/**
 * @file
 * Retrieve, save, and remove patterns from the database.
 */

/**
 * Returns a pattern object from different input parameters.
 * 
 * Returns FALSE in case of errors.
 * 
 * @param mixed $pattern A pattern object, an array representing
 *  the pattern object, a numeric id or alphanumeric name of 
 *  the pattern as it is in the database
 *  
 */
function _patterns_db_get_pattern($pattern) {
  if (is_null($pattern)) return FALSE;
  if (is_array($pattern)) {
    $pattern = (object) $pattern;
  }
  if (!is_object($pattern)) {
    $pattern = patterns_get_pattern($pattern);
  }
  return $pattern;
}

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
  if (empty($id)) return FALSE;
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
  if (empty($name)) return FALSE;
  
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

/**
 * Returns the array of patterns objects that are currently enabled.
 * 
 * It is possible to specify which fields to return with the fields 
 * parameter, that can be either a string or an array.
 * 
 * The pattern code within each each object is still to be unserialized.
 * 
 * @param mixed $fields Specifies which fields to return.
 * 
 */
function patterns_db_get_enabled_patterns($fields = array()) {
  if (!empty($fields)) {
    if (!is_array($fields)) {
      $fields = array($fields);
    }
  }
  $query = db_select('patterns', 'p');
  $result = $query->condition('status', PATTERNS_STATUS_ENABLED)
                  ->fields('p', $fields)
                  ->execute()
                  ->fetchAll();
                  
  return $result;
}


/**
 * Returns the array of patterns array that are currently enabled.
 * 
 * It is possible to specify which fields to return with the fields 
 * parameter, that can be either a string or an array.
 * 
 * If found, pattern code inside each pattern object is
 * automatically unserialized.
 * 
 * @param mixed $fields Specifies which fields to return.
 * 
 */
function patterns_db_get_enabled_patterns_array($fields = array()) {
  $patterns = patterns_db_get_enabled_patterns($fields);
  $out = array();
  foreach($patterns as $p) {
    $pa = get_object_vars($p);
    if (isset($pa['pattern'])) {
      $pa = unserialize($pa['pattern']);
    }
    $out[] = $pa;
  }
  return $out;
}

/**
 * Compares the date of the last update in the database with
 * the one in the file system and return TRUE if the file system
 * has a newer copy of the pattern stored in the database.
 * 
 * Return TRUE also if it cannot finds the file in the fs.
 * 
 * @param mixed $pattern A pattern object, an array representing
 *  the pattern object, a numeric id or alphanumeric name of 
 *  the pattern as it is in the database
 * 
 */
function patterns_db_is_pattern_updated ($pattern = NULL) {
  $pattern = _patterns_db_get_pattern($pattern);
  if (empty($pattern)) return FALSE;
  
  if (!file_exists($pattern->file)) return TRUE;
  $new_updated = filemtime($pattern->file);
  $updated = $pattern->updated;
  
  if ($new_updated && !$updated) return TRUE;
  if ($updated && !$new_updated) return FALSE;
  if (!$updated && !$new_updated) return FALSE;
  
  if ($new_updated > $updated) return TRUE;
}



/**
 * Looks up a numerical id into the database for a valid
 * patterns name. Returns false, if no valid pattern name
 * was found.
 * 
 * 
 * @param mixed $name the name of the pattern to look up
 *  
 */
function patterns_db_get_id_from_name ($name) {
  if (empty($name)) return FALSE;
  $p = patterns_get_pattern($name);
  if ($p) $p = $p['id'];
  return $p;
}


/**
 * Looks up a pattern name into the database and returns
 * the numerical id associated with it. Returns false, 
 * if no valid pattern name was passed.
 * 
 * @param mixed $id the numerical id to look up
 * 
 */
function patterns_db_get_name_from_id ($id) {
  if (empty($id)) return FALSE;
  $p = patterns_get_pattern($id);
  if ($p) $p = $p['name'];
  return $p;
}



/**
 * Returns a string representation of the status of the pattern as it 
 * is in the database;
 * 
 * @param mixed $pattern A pattern object, an array representing
 *  the pattern object, a numeric id or alphanumeric name of 
 *  the pattern as it is in the database
 */
function patterns_db_analyze_patterns_status ($pattern) {
  $pattern = _patterns_db_get_pattern($pattern);
  
  if (!$pattern) return 'This pattern has not been validated yet.';
  if ($pattern->status >= 0) return 'This is a valid pattern';
  if ($pattern->status === PATTERNS_STATUS_INVALID) return 'This pattern is marked as NOT valid';
  if ($pattern->status === PATTERNS_STATUS_REMOVED) return 'This pattern is marked as REMOVED.';
  
  return 'This pattern has not been validated yet.';
}
