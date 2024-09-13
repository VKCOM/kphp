@kphp_should_fail k2_skip
/Invalid php2c conversion in this context: FFI\\CData_float -> double/
<?php

$cdef = FFI::cdef('struct Foo { double field; };');

$foo = $cdef->new('struct Foo');
$float = FFI::new('float');
$foo->field = $float;
