@kphp_should_fail
KPHP_ENABLE_FFI=1
/casting a non-scalar type cdata\$cdef.*\\Foo to a scalar type int32_t/
<?php

$cdef = FFI::cdef('struct Foo { int x; };');
$foo = $cdef->new('struct Foo');
$as_int = FFI::cast('int', $foo);
