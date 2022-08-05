@kphp_should_fail
/Invalid php2c conversion in this context: int -> float/
<?php

$cdef = FFI::cdef('struct Foo { float field; };');

$foo = $cdef->new('struct Foo');
$foo->field = 0;
