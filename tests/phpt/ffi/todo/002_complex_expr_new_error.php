@kphp_should_fail
KPHP_ENABLE_FFI=1
<?php

function test() {
  $cdef = FFI::cdef('struct Foo { int i; };');
  var_dump($cdef->new('struct Foo')->i);
  $cdef->new('struct Foo')->i = 10;
}

test();
