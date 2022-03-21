<?php

function test() {
  $cdef = FFI::cdef('
    struct Foo {
      int8_t x;
      int32_t y;
    };

    struct Bar {
      char x;
    };

    struct Baz {
      struct Foo foo;
      struct Bar bar;
    };

    struct Qux {
      struct Baz baz;
    };

    struct WithArrays {
      struct Qux qux_arr[3];
      uint8_t data[100];
    };
  ');

  $foo = $cdef->new('struct Foo');
  $bar = $cdef->new('struct Bar');
  $baz = $cdef->new('struct Baz');
  $qux = $cdef->new('struct Qux');
  var_dump(FFI::sizeof($foo));
  var_dump(FFI::sizeof(FFI::addr($foo)));
  var_dump(FFI::sizeof($bar));
  var_dump(FFI::sizeof($baz));
  var_dump(FFI::sizeof($baz->foo));
  var_dump(FFI::sizeof($baz->bar));
  var_dump(FFI::sizeof($qux));
  var_dump(FFI::sizeof($qux->baz));
  var_dump(FFI::sizeof($qux->baz->foo));
  var_dump(FFI::sizeof($qux->baz->bar));

  // test fixed-size numeric types
  $int8 = FFI::new('int8_t');
  $int16 = FFI::new('int16_t');
  var_dump(FFI::sizeof($int8));
  var_dump(FFI::sizeof(FFI::addr($int8)));
  var_dump(FFI::sizeof($int16));
  var_dump(FFI::sizeof(FFI::addr($int16)));

  $ptr = FFI::addr($int16);
  var_dump(FFI::sizeof(FFI::addr($ptr)));

  // now test arch/os-dependent type sizes
  $size_t = FFI::new('size_t');
  $int = FFI::new('int');
  $unsigned = FFI::new('unsigned');
  $long = FFI::new('long');
  $long_long = FFI::new('long long');
  $unsigned_long = FFI::new('unsigned long');
  $unsigned_long_long = FFI::new('unsigned long long');
  $short = FFI::new('short');
  var_dump(FFI::sizeof($size_t));
  var_dump(FFI::sizeof($int));
  var_dump(FFI::sizeof($unsigned));
  var_dump(FFI::sizeof($long));
  var_dump(FFI::sizeof($long_long));
  var_dump(FFI::sizeof($unsigned_long));
  var_dump(FFI::sizeof($unsigned_long_long));
  var_dump(FFI::sizeof($short));

  // tests for arrays
  $with_arrays = $cdef->new('struct WithArrays');
  $arr = FFI::new('int[14]');
  $arr2_size = 21;
  $arr2 = FFI::new('double[' . $arr2_size . ']');
  var_dump(FFI::sizeof($arr));
  var_dump(FFI::sizeof($arr2));
  var_dump(FFI::sizeof($with_arrays));
  var_dump(FFI::sizeof($with_arrays->qux_arr));
  var_dump(FFI::sizeof($with_arrays->data));
}

test();
