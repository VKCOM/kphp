@kphp_should_fail
KPHP_ENABLE_FFI=1
/FFI::load\(\): line 3: syntax error, unexpected INT_CONSTANT/
<?php

$cdef = FFI::load(__DIR__ . '/bad_header.h');
