@kphp_should_fail
/FFI callback should not throw/
/Throw chain: function\(\) -> throwing/
<?php

function throwing() {
  throw new \Exception('oops');
}

function test() {
  $cdef = FFI::cdef('
    void f(int (*g) ());
  ');

  $cdef->f(function () {
    throwing();
    return 1;
  });
}

test();
