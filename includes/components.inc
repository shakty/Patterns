<?php
/*
 * @file
 * Retrieves, sets information about Patterns components.
 *
 */

function patterns_components_list($reset = FALSE) {
	$components = patterns_io_load_components($reset);
	
	return $components;
}
