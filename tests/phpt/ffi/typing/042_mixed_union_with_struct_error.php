@kphp_should_fail k2_skip
/Mismatched union/struct tag for ffi_cdata<example, Foo>/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  union Foo { int x; };
');

/** @param ffi_cdata<example, struct Foo> $x */
function f($x) {}

$foo = $cdef->new('union Foo');
f($foo);
