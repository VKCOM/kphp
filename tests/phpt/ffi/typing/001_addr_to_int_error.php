@kphp_should_fail
KPHP_ENABLE_FFI=1
/pass FFI\\CData_int64\* to argument/
/declared as @param int/
<?php

function f(int $x) {}

$cdef = FFI::cdef('struct Foo { int64_t x; };');
$foo = $cdef->new('struct Foo');
f(FFI::addr($foo->x));
