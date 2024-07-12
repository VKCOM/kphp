@kphp_should_fail k2_skip
/FFI\\CData_int32 from function/
/declared as @return FFI\\CData_void\*/
<?php

function test() {
  $cdef = FFI::cdef('
    void f(void* (*g) ());
  ');
  $cdef->f(function () {
    $v = FFI::new('int');
    return $v;
  });
}

test();
