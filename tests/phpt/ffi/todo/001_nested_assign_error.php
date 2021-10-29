@kphp_should_fail
KPHP_ENABLE_FFI=1
<?php

// Doesn't work right now, but should probably work in the future.

$cdef = FFI::cdef('struct Foo { int x; };');
$foo2 = $foo = $cdef->new('struct Foo');
$foo2->x = 10;
var_dump($foo2->x);
