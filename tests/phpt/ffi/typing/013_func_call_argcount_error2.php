@kphp_should_fail
/Too few arguments in call to scope\\example::f\(\), expected 2, have 1/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f(int x, int y);
');

$cdef->f(1);
