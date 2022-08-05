@kphp_should_fail
/FFI::cdef\(\): line 1: syntax error, unexpected INT_CONSTANT/
<?php

$cdef = FFI::cdef('123');
