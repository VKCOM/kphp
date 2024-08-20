@kphp_should_fail k2_skip
/Method g\(\) not found in class ffi_scope<example>/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f();
');

$cdef->g();
