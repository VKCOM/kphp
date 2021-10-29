<?php

function test() {
  $cdef = FFI::cdef('
    struct Foo {
      int8_t value;
    };
    struct Bar {
      struct Foo *fooptr;
    };
  ');

  $bar = $cdef->new('struct Bar');
  $fooptr = $bar->fooptr;
  var_dump($bar->fooptr === null);
  var_dump($fooptr === null);
  var_dump(FFI::isNull($bar->fooptr));
  $bar->fooptr = null;
  var_dump($bar->fooptr === null);
  var_dump(FFI::isNull($bar->fooptr));

  var_dump(FFI::isNull(FFI::addr($bar)));

  var_dump(FFI::isNull(FFI::new("int*")));

  $i = FFI::new("int");
  var_dump(FFI::isNull(FFI::addr($i)));
}

test();
