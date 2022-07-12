@kphp_should_fail
KPHP_ENABLE_FFI=1
/pass \?string to argument \$s of ensure_string/
<?php

function ensure_string(string $s) {}

function test() {
  $cdef = FFI::cdef('
    const char *f();
  ');
  ensure_string($cdef->f());
}

test();
