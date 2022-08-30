@kphp_should_fail
/f: @kphp-ffi-signalsafe should not be used with FFI callbacks/
<?php

function test() {
  $cdef = FFI::cdef('
    // also test multi-line comment
    // @kphp-ffi-signalsafe
    void f(int (*g) ());
  ');

  $cdef->f(function () { return 0; });
}

test();
