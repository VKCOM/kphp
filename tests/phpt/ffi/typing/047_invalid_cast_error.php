@kphp_should_fail
KPHP_ENABLE_FFI=1
/FFI::cast\(\): line 1: syntax error/
<?php

$int8 = FFI::new('int8_t');
$_ = FFI::cast('badtype', $int8);
