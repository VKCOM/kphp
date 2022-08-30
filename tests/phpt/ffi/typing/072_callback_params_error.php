@kphp_should_fail
/pass int to argument/
/declared as @param Foo/
<?php

class Foo {}

function test() {
  $cdef = FFI::cdef('
    void f(void (*g) (int x));
  ');
  $cdef->f(function (Foo $x) {});
}

test();
