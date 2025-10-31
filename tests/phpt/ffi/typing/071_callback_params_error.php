@kphp_should_fail k2_skip
Too few arguments for callback \(callable\(int\): void\), expected 1, have 0
<?php

function test() {
  $cdef = FFI::cdef('
    void f(void (*g) (int x));
  ');
  $cdef->f(function () {});
}

test();
