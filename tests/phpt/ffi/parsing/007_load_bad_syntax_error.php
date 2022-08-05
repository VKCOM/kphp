@kphp_should_fail
/FFI::load\(\): line 3: syntax error, unexpected INT_CONSTANT/
<?php

$cdef = FFI::load(__DIR__ . '/bad_header.h');
