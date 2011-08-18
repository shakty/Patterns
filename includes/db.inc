<?php 

/**
 * 
 * Remove a pattern from the database.
 * 
 * @param mixed $pid the id of the pattern to remove
 */
function patterns_db_remove_pattern($pid) {
  
  $del = db_delete('patterns')
    ->condition('pid', $pid)
    ->execute();
  

  return ($del==1) ? TRUE : FALSE;
}

