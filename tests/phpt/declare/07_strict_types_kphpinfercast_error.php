@kphp_should_fail
/pass string to argument \$s of infercast_int_param/
/but it's declared as @param int/
<?php

require_once __DIR__ . '/infercast_func.php.inc';
require_once __DIR__ . '/can_call_with_cast.php.inc'; // OK
require_once __DIR__ . '/cant_call_with_cast.php.inc'; // Error
