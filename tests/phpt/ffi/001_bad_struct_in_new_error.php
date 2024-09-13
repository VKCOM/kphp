@kphp_should_fail k2_skip
/ffi->new\(\): line 1: syntax error, unexpected IDENTIFIER/
<?php

// Like in PHP (but basically in C) we don't create a named type
// for `struct Foo`; one needs to use a typedef to make it work
// without `struct` prefix.

$cdef = FFI::cdef('
  struct Foo { bool field; };
');

$foo = $cdef->new('Foo');
