@kphp_should_fail k2_skip
/pass const FFI\\CData_int32\* to argument \$x of f/
/but it's declared as @param Foo/
<?php

class Foo {}

/** @param Foo $x */
function f($x) {}

$const_intptr = FFI::new('const int32_t*');

f($const_intptr);
