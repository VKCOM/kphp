@kphp_should_fail k2_skip
/pass FFI\\CData_int64 to argument \$x of expect_int32/
/declared as @param FFI\\CData_int32/
<?php

/** @param \FFI\CData_int32 $x */
function expect_int32($x) {}

$int64 = FFI::new('int64_t');
expect_int32($int64);
