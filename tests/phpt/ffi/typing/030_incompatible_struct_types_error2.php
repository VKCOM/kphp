@kphp_should_fail k2_skip
/pass cdata\$test\\Foo to argument/
/declared as @param cdata\$test\\Bar/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "test"
  struct Foo { char data; };
  struct Bar { char data; };
');

/** @param ffi_cdata<test, struct Bar> $x */
function f($x) {}

$foo = $cdef->new('struct Foo');
f($foo);
