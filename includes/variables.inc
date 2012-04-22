<?php
/**
 * @file
 * A file containing a set of constants and variables.
 *
 */

static $patterns_formats = array();

define('PATTERNS_PUBLIC_URL', 'patterns_public_url');

define('PATTERNS_UPDATES', 'patterns_updates');

// Notice: if you change this value you must update also patterns.test
define('PATTERNS_FIRST_INSTALL', 'patterns_first_install');

define('PATTERNS_PARSER_PARSE', 'parse');
define('PATTERNS_PARSER_DUMP', 'dump');
define('PATTERNS_PARSER_DUMP_COMMENT', 'dump_comment');
define('PATTERNS_PARSER_LOAD', 'load');

define('PATTERNS_FORMAT_UNKNOWN', 'unknown');

define('PATTERNS_SUCCESS', 1);
define('PATTERNS_WARN', 0);
define('PATTERNS_ERR', -1);

define('PATTERNS_TAG', 'tag');
define('PATTERNS_SECTION_INFO', 'info');
define('PATTERNS_SECTION_MODULES', 'modules');

define('PATTERNS_CREATE', 'create');
define('PATTERNS_MODIFY', 'modify');
define('PATTERNS_DELETE', 'delete');
define('PATTERNS_INCLUDE', 'include');

define('PATTERNS_EXPORT', 'export');
define('PATTERNS_EXPORT_ALL', 'All');
define('PATTERNS_EXTRACT_ALL', 'extract_all');

define('PATTERNS_FILES', 'files');
define('PATTERNS_INFO', 'descr');


define('PATTERNS_STATUS_OK', 0);        // installed, not enabled
define('PATTERNS_STATUS_ENABLED', 1);   // installed, enabled
define('PATTERNS_STATUS_INVALID', -1);  // invalid
define('PATTERNS_STATUS_TRASHED', -5);  // removed

define('PATTERNS_INCLUDE_NEVER', 'never');
define('PATTERNS_INCLUDE_FIRSTRUN', 'firstrun');
define('PATTERNS_INCLUDE_UPDATE', 'onupdate');
define('PATTERNS_INCLUDE_FIRSTRUN_OR_UPDATE', 'first_or_up');
define('PATTERNS_INCLUDE_ATTACHED', 'attached');
define('PATTERNS_INCLUDE_ALWAYS', 'always');

define('PATTERNS_EXPORT_TO_FILE', 'file');
define('PATTERNS_EXPORT_TO_DB', 'database');
define('PATTERNS_EXPORT_TO_ZIP', 'zip');

define('PATTERNS_EXPORT_MODE_FULL', 'full');
define('PATTERNS_EXPORT_MODE_ACTIONS', 'actions');
define('PATTERNS_EXPORT_MODE_ID', 'id');
define('PATTERNS_EXPORT_MODE_NAME', 'name');

define('PATTERNS_ALLOW_PUBLISH_DEFAULT', TRUE);
