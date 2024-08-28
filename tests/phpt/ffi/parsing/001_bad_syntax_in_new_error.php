@kphp_should_fail k2_skip
/ffi->new\(\): line 1: syntax error, unexpected INT_CONSTANT/
<?php

$cdef = FFI::cdef('
  struct Foo { bool field; };
');

$foo = $cdef->new('123');
