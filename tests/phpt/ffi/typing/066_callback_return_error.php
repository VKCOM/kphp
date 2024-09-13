@kphp_should_fail k2_skip
/return int from function/
/declared as @return void/
<?php

function test() {
  $cdef = FFI::cdef('
    void f(void (*g) ());
  ');
  $cdef->f(function () {
    return 10;
  });
}

test();
