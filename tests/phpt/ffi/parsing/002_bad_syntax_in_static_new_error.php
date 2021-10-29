@kphp_should_fail
KPHP_ENABLE_FFI=1
/FFI::new\(\): line 1: syntax error, unexpected INT_CONSTANT/
<?php

$foo = FFI::new('123');
