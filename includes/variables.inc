<?php

define('PATTERNS_SUCCESS', 1);
define('PATTERNS_WARN', 0);
define('PATTERNS_ERR', -1);

define('PATTERNS_CREATE', 'create');
define('PATTERNS_MODIFY', 'modify');
define('PATTERNS_DELETE', 'delete');
define('PATTERNS_INCLUDE', 'include');

define('PATTERNS_FILES', 'files');
define('PATTERNS_INFO', 'descr');


define('PATTERNS_STATUS_OK', 0);        // installed, not enabled
define('PATTERNS_STATUS_ENABLED', 1);   // installed, enabled
define('PATTERNS_STATUS_INVALID', -1);  // invalid
define('PATTERNS_STATUS_REMOVED', -5);  // removed

define('PATTERNS_INCLUDE_NEVER', 'never');
define('PATTERNS_INCLUDE_FIRSTRUN', 'firstrun');
define('PATTERNS_INCLUDE_UPDATE', 'onupdate');
define('PATTERNS_INCLUDE_FIRSTRUN_OR_UPDATE', 'first_or_up');
define('PATTERNS_INCLUDE_ATTACHED', 'attached');
define('PATTERNS_INCLUDE_ALWAYS', 'always');

define('PATTERNS_INCLUDE_NEVER_TEXT', 'Never run');
define('PATTERNS_INCLUDE_FIRSTRUN_TEXT', 'Only during the first run');
define('PATTERNS_INCLUDE_UPDATE_TEXT', 'Only if pattern was updated');
define('PATTERNS_INCLUDE_FIRSTRUN_OR_UPDATE_TEXT', 'If it is the first run or if the pattern was updated');
define('PATTERNS_INCLUDE_ATTACHED_TEXT', 'Only if the pattern code is attached (no lookup to database)');
define('PATTERNS_INCLUDE_ALWAYS_TEXT', 'Always');

define('PATTERNS_EXPORT_TO_FILE', 'file');
define('PATTERNS_EXPORT_TO_DB', 'database');
define('PATTERNS_EXPORT_TO_ZIP', 'zip');

define('PATTERNS_EXPORT_TO_FILE_TEXT', 'Download as a pattern file');
define('PATTERNS_EXPORT_TO_DB_TEXT', 'Import into the database');
define('PATTERNS_EXPORT_TO_ZIP_TEXT', 'Download as a zip-archive');


define('PATTERNS_EXPORT_MODE_FULL', 'full');
define('PATTERNS_EXPORT_MODE_ACTIONS', 'actions');
define('PATTERNS_EXPORT_MODE_ID', 'id');
define('PATTERNS_EXPORT_MODE_NAME', 'name');

define('PATTERNS_EXPORT_MODE_FULL_TEXT', 'All the sections (info and modules included) of the pattern');
define('PATTERNS_EXPORT_MODE_ACTIONS_TEXT', 'Actions only (info and modules sections excluded) of the pattern');
define('PATTERNS_EXPORT_MODE_ID_TEXT', 'The numerical id of the pattern');
define('PATTERNS_EXPORT_MODE_NAME_TEXT', 'The unique name of the pattern');
