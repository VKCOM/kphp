@kphp_should_fail
KPHP_ENABLE_FFI=1
/Invalid property access ...->cdata: does not exist in class/
<?php

$cdef = FFI::cdef('struct Foo { int x; };');
$foo = $cdef->new('struct Foo');
$foo->x->cdata = 53;
