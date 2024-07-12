@kphp_should_fail k2_skip
/Invalid php2c conversion: cdata\$example\\Bar -> struct Foo/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  struct Foo { int8_t field; };
  struct Bar { int8_t field; };
  void f(struct Foo x);
');

$bar = $cdef->new('struct Bar');
$cdef->f($bar);
