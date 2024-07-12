@kphp_should_fail k2_skip
/Invalid php2c conversion in this context: int -> double/
<?php

$cdef = FFI::cdef('struct Foo { double field; };');

$foo = $cdef->new('struct Foo');
$foo->field = 0;
