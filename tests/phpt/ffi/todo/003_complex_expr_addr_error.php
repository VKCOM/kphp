@kphp_should_fail
KPHP_ENABLE_FFI=1
<?php

function test() {
  $cdef = FFI::cdef('struct Foo { int i; };');
  var_dump(FFI::addr($cdef->new('struct Foo'))->i);
  FFI::addr($cdef->new('struct Foo'))->i = 20;
}

test();
