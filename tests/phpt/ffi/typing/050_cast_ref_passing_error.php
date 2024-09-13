@kphp_should_fail k2_skip
/pass &FFI\\CData_int32 to argument/
/declared as @param FFI\\CData_int32/
<?php

/** @param \FFI\CData_int32 $x */
function f($x) {}

$int64 = FFI::new('int64_t');
$int32 = FFI::cast('int32_t', $int64);
f($int32);
