<?php

// Overflow and underflow tests.

function test() {
  $cdef = FFI::cdef('struct NumTypes {
    int8_t i8;
    int16_t i16;
    int32_t i32;
    int64_t i64;

    uint8_t ui8;
    uint16_t ui16;
    uint32_t ui32;
    uint64_t ui64;
  };
  ');
  $x = $cdef->new('struct NumTypes');

  $values = [
    1, -1,
    300, -200,
    35000, -38435,
    2147483649, -2157483649,
    3147483649, -4647483649,
  ];

  foreach ($values as $v) {
    $x->i8 = $v;
    $x->i16 = $v;
    $x->i32 = $v;
    $x->i64 = $v;
    $x->ui8 = $v;
    $x->ui16 = $v;
    $x->ui32 = $v;
    $x->ui64 = $v;
    var_dump([
      'value' => $v,
      'i8' => $x->i8,
      'i16' => $x->i16,
      'i32' => $x->i32,
      'i64' => $x->i64,
      'ui8' => $x->ui8,
      'ui16' => $x->ui16,
      'ui32' => $x->ui32,
      'ui64' => $x->ui64,
    ]);
  }
}

test();
