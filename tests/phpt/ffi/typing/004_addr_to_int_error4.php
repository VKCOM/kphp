@kphp_should_fail
/pass FFI\\CData_int64\*\* to argument/
/declared as @param int/
<?php

function f(int $x) {}

$int = FFI::new('int64_t');
$int_ptr = FFI::addr($int);
f(FFI::addr($int_ptr));
