@kphp_should_fail k2_skip
/pass FFI\\CData_int16\* to argument/
/declared as @param FFI\\CData_int8\*/
<?php

$cdef = FFI::cdef('
  struct Foo {
    int16_t *x;
  };
  int f(int8_t* ptr);
');

$foo = $cdef->new('struct Foo');
$cdef->f($foo->x);
