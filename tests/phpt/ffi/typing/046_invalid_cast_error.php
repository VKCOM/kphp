@kphp_should_fail
/casting a non-scalar type struct Foo to a scalar type int32_t/
<?php

$cdef = FFI::cdef('struct Foo { int x; };');
$foo = $cdef->new('struct Foo');
$as_int = FFI::cast('int', $foo);
