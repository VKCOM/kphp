@kphp_should_fail
KPHP_ENABLE_FFI=1
/duplicate definition of `example` scope/
<?php

function test1() {
  $cdef = FFI::cdef('
    #define FFI_SCOPE "example"
    struct Foo { int8_t x; };
  ');
}

function test2() {
  $cdef = FFI::cdef('
    #define FFI_SCOPE "example"
    struct Bar { int8_t y; };
  ');
}

test1();
test2();
