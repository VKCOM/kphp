@kphp_should_fail k2_skip
/return FFI\\CData_int32\* from function/
/declared as @return FFI\\CData_int32\*\*/
<?php

function test() {
  $cdef = FFI::cdef('
    void f(int** (*g) ());
  ');
  $cdef->f(function () {
    $v = FFI::new('int*');
    return $v;
  });
}

test();
