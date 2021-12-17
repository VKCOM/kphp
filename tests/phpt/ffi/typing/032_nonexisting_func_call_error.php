@kphp_should_fail
KPHP_ENABLE_FFI=1
/Method g\(\) not found in class ffi_scope<example>/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  void f();
');

$cdef->g();
