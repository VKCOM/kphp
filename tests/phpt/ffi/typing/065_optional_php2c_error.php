@kphp_should_fail k2_skip
/Invalid php2c conversion: int\|false -> int64_t/
<?php

/**
 * @param int|false $optional_int
 */
function test($optional_int = false) {
  $cdef = FFI::cdef('
    void f(int64_t x);
  ');
  $cdef->f($optional_int);
}

test(false);
test(1);
