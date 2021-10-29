<?php

function test() {
  $cdef = FFI::cdef('struct Foo { int8_t x; };');
  $int = FFI::new('int');
  $foo = $cdef->new('struct Foo');

  var_dump(get_class($int));
  var_dump(get_class($foo));
  var_dump(get_class(FFI::addr($int)));
  var_dump(get_class(FFI::addr($foo)));
}

test();
