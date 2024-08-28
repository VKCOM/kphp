@kphp_should_fail k2_skip
/Callbacks passed to builtin functions must not be resumable/
<?php

function resumable_func() {
  return 1;
}

function test() {
  $cdef = FFI::cdef('
    void f(int (*g) ());
  ');

  $cdef->f('resumable_func');
}

if (false) {
  fork(resumable_func());
}
test();
