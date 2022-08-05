@kphp_should_fail
/Field \$cdata not found in class FFI\\CData_int32/
<?php

$cdef = FFI::cdef('struct Foo { int x; };');
$foo = $cdef->new('struct Foo');
$foo->x->cdata = 53;
