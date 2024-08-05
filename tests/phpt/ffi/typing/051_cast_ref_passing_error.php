@kphp_should_fail k2_skip
/pass &cdata\$example\\Bar to argument \$bar of f/
/declared as @param cdata\$example\\Bar/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  struct Foo { int32_t x; };
  struct Bar { int32_t x; };
');

/** @param ffi_cdata<example, struct Bar> $bar */
function f($bar) {}

$foo = $cdef->new('struct Foo');
$as_bar = $cdef->cast('struct Bar', $foo);
f($as_bar);
