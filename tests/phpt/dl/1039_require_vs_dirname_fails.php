@kphp_should_fail
/call to 'dirname' must have one arg/
/'dirname' has to be called with string type arg/
<?php
require_once dirname(__FILE__, 2) . '/include/foo.php';
require_once dirname(123) . '/include/foo.php';
