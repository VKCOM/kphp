@kphp_should_fail k2_skip
/Too many arguments in call to scope\\example::f\(\), expected 0, have 2/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f();
');

$cdef->f(1, 2);
