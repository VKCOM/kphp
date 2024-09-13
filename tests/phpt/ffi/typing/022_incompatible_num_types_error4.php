@kphp_should_fail k2_skip
/Invalid php2c conversion in this context: FFI\\CData_int16 -> int8_t/
<?php

$cdef = FFI::cdef('struct Foo { int8_t i8_field; };');

$foo = $cdef->new('struct Foo');
$foo->i8_field = FFI::new('int16_t');
