@kphp_should_fail k2_skip
/pass &cdata\$test\\Foo to argument/
<?php

/** @param ffi_cdata<test, struct Foo> $foo */
function f($foo) {}

$cdef = FFI::cdef('
  #define FFI_SCOPE "test"
  struct Foo { int8_t x; };
  struct Bar { struct Foo foo; };
');
$bar = $cdef->new('struct Bar');
f($bar->foo);
