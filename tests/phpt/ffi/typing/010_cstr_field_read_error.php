@kphp_should_fail
KPHP_ENABLE_FFI=1
/pass FFI\\CData_char\* to argument/
/declared as @param string/
<?php

function test() {
  $cdef = FFI::cdef('
    struct Foo {
      const char *s;
    };
  ');
  $foo = $cdef->new('struct Foo');
  f($foo->s);
}

function f(string $s) {}

test();
