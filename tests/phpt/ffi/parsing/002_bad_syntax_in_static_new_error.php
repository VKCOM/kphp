@kphp_should_fail
/FFI::new\(\): line 1: syntax error, unexpected INT_CONSTANT/
<?php

$foo = FFI::new('123');
