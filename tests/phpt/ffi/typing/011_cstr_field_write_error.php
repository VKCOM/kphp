@kphp_should_fail
KPHP_ENABLE_FFI=1
/Invalid php2c conversion in this context: string -> const char\*/
<?php

function test() {
  $cdef = FFI::cdef('
    struct Foo {
      const char *s;
    };
  ');
  $foo = $cdef->new('struct Foo');
  $foo->s = "hello";
}

test();
