@kphp_should_fail k2_skip
/Field \$cdata not found in class FFI\\CData_int32/
<?php

$cdef = FFI::cdef('struct Foo { int x; };');
$foo = $cdef->new('struct Foo');
var_dump($foo->x->cdata);
