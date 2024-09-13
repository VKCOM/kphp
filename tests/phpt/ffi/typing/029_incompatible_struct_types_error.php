@kphp_should_fail k2_skip
/Invalid php2c conversion: cdata\$test\\Foo -> struct Bar/
<?php

$cdef = FFI::cdef('
  #define FFI_SCOPE "test"
  struct Foo { char data; };
  struct Bar { char data; };
  void f_bar(struct Bar x);
');

$foo = $cdef->new('struct Foo');
$cdef->f_bar($foo);
