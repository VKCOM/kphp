@kphp_should_fail
KPHP_ENABLE_FFI=1
/FFI::cdef\(\): line 1: syntax error, unexpected INT_CONSTANT/
<?php

$cdef = FFI::cdef('123');
