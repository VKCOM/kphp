@kphp_should_fail k2_skip
/Invalid php2c conversion in this context: int -> float/
<?php

$cdef = FFI::cdef('struct Foo { float field; };');

$foo = $cdef->new('struct Foo');
$foo->field = 0;
