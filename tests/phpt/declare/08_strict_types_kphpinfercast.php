@ok
<?php

// infercast_func.php uses strict_types=1, but it doesn't apply here

require_once __DIR__ . '/infercast_func.php.inc';
require_once __DIR__ . '/can_call_with_cast.php.inc';

infercast_string_param(43);
infercast_int_param('foo');
