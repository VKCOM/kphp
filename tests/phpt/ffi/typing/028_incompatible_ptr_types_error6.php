@kphp_should_fail k2_skip
/pass FFI\\CData_int8\*\* to argument/
/declared as @param FFI\\CData_int8\*/
<?php

$cdef = FFI::cdef('
  struct Foo {
    int8_t **x;
  };
  int f(int8_t *ptr);
');

$foo = $cdef->new('struct Foo');
$cdef->f($foo->x);
