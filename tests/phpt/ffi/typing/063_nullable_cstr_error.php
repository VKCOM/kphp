@kphp_should_fail
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
