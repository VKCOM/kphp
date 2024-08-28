@kphp_should_fail k2_skip
/pass const FFI\\CData_int32\* to argument \$x of f/
/but it's declared as @param FFI\\CData_int32\*/
<?php

// Passing `const T*` as `T*` is a type error.

/** @param ffi_cdata<C, int32_t*> $x */
function f($x) {}

$intptr = FFI::new('int32_t*');
$const_intptr = FFI::new('const int32_t*');

f($const_intptr);
f($intptr);
