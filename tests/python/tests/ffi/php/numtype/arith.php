<?php

// Basic arith operation tests just to make sure we get the conversions right.
// For '/' operations we add int casts as division yields float in KPHP.

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

  $numtypes = $cdef->new('struct NumTypes');
  $numtypes->i8 += 2;
  $numtypes->i16 -= 16;
  $numtypes->i16 *= 2;
  var_dump([$numtypes->i8, $numtypes->i16]);

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

    var_dump(['$x->i8 << 2' => $x->i8 << 2, 'value' => $v]);
    var_dump(['$x->i16 << 2' => $x->i16 << 2, 'value' => $v]);
    var_dump(['$x->i32 << 2' => $x->i32 << 2, 'value' => $v]);
    var_dump(['$x->i64 << 2' => $x->i64 << 2, 'value' => $v]);
    var_dump(['$x->ui8 << 2' => $x->ui8 << 2, 'value' => $v]);
    var_dump(['$x->ui16 << 2' => $x->ui16 << 2, 'value' => $v]);
    var_dump(['$x->ui32 << 2' => $x->ui32 << 2, 'value' => $v]);
    var_dump(['$x->ui64 << 2' => $x->ui64 << 2, 'value' => $v]);

    var_dump(['$x->i8 >> 2' => $x->i8 >> 2, 'value' => $v]);
    var_dump(['$x->i16 >> 2' => $x->i16 >> 2, 'value' => $v]);
    var_dump(['$x->i32 >> 2' => $x->i32 >> 2, 'value' => $v]);
    var_dump(['$x->i64 >> 2' => $x->i64 >> 2, 'value' => $v]);
    var_dump(['$x->ui8 >> 2' => $x->ui8 >> 2, 'value' => $v]);
    var_dump(['$x->ui16 >> 2' => $x->ui16 >> 2, 'value' => $v]);
    var_dump(['$x->ui32 >> 2' => $x->ui32 >> 2, 'value' => $v]);
    var_dump(['$x->ui64 >> 2' => $x->ui64 >> 2, 'value' => $v]);

    // Auto-generated combinations for + - * /

    var_dump(['$x->i8 + $x->i8' => $x->i8 + $x->i8, 'value' => $v]);
    var_dump(['$x->i8 - $x->i8' => $x->i8 - $x->i8, 'value' => $v]);
    var_dump(['$x->i8 / $x->i8' => (int)($x->i8 / $x->i8), 'value' => $v]);
    var_dump(['$x->i8 + $x->i16' => $x->i8 + $x->i16, 'value' => $v]);
    var_dump(['$x->i8 - $x->i16' => $x->i8 - $x->i16, 'value' => $v]);
    var_dump(['$x->i8 / $x->i16' => (int)($x->i8 / $x->i16), 'value' => $v]);
    var_dump(['$x->i8 + $x->i32' => $x->i8 + $x->i32, 'value' => $v]);
    var_dump(['$x->i8 - $x->i32' => $x->i8 - $x->i32, 'value' => $v]);
    var_dump(['$x->i8 / $x->i32' => (int)($x->i8 / $x->i32), 'value' => $v]);
    var_dump(['$x->i8 + $x->i64' => $x->i8 + $x->i64, 'value' => $v]);
    var_dump(['$x->i8 - $x->i64' => $x->i8 - $x->i64, 'value' => $v]);
    var_dump(['$x->i8 / $x->i64' => (int)($x->i8 / $x->i64), 'value' => $v]);
    var_dump(['$x->i8 + $x->ui8' => $x->i8 + $x->ui8, 'value' => $v]);
    var_dump(['$x->i8 - $x->ui8' => $x->i8 - $x->ui8, 'value' => $v]);
    var_dump(['$x->i8 / $x->ui8' => (int)($x->i8 / $x->ui8), 'value' => $v]);
    var_dump(['$x->i8 + $x->ui16' => $x->i8 + $x->ui16, 'value' => $v]);
    var_dump(['$x->i8 - $x->ui16' => $x->i8 - $x->ui16, 'value' => $v]);
    var_dump(['$x->i8 / $x->ui16' => (int)($x->i8 / $x->ui16), 'value' => $v]);
    var_dump(['$x->i8 + $x->ui32' => $x->i8 + $x->ui32, 'value' => $v]);
    var_dump(['$x->i8 - $x->ui32' => $x->i8 - $x->ui32, 'value' => $v]);
    var_dump(['$x->i8 / $x->ui32' => (int)($x->i8 / $x->ui32), 'value' => $v]);
    var_dump(['$x->i8 + $x->ui64' => $x->i8 + $x->ui64, 'value' => $v]);
    var_dump(['$x->i8 - $x->ui64' => $x->i8 - $x->ui64, 'value' => $v]);
    var_dump(['$x->i8 / $x->ui64' => (int)($x->i8 / $x->ui64), 'value' => $v]);
    var_dump(['$x->i16 + $x->i8' => $x->i16 + $x->i8, 'value' => $v]);
    var_dump(['$x->i16 - $x->i8' => $x->i16 - $x->i8, 'value' => $v]);
    var_dump(['$x->i16 / $x->i8' => (int)($x->i16 / $x->i8), 'value' => $v]);
    var_dump(['$x->i16 + $x->i16' => $x->i16 + $x->i16, 'value' => $v]);
    var_dump(['$x->i16 - $x->i16' => $x->i16 - $x->i16, 'value' => $v]);
    var_dump(['$x->i16 / $x->i16' => (int)($x->i16 / $x->i16), 'value' => $v]);
    var_dump(['$x->i16 + $x->i32' => $x->i16 + $x->i32, 'value' => $v]);
    var_dump(['$x->i16 - $x->i32' => $x->i16 - $x->i32, 'value' => $v]);
    var_dump(['$x->i16 / $x->i32' => (int)($x->i16 / $x->i32), 'value' => $v]);
    var_dump(['$x->i16 + $x->i64' => $x->i16 + $x->i64, 'value' => $v]);
    var_dump(['$x->i16 - $x->i64' => $x->i16 - $x->i64, 'value' => $v]);
    var_dump(['$x->i16 / $x->i64' => (int)($x->i16 / $x->i64), 'value' => $v]);
    var_dump(['$x->i16 + $x->ui8' => $x->i16 + $x->ui8, 'value' => $v]);
    var_dump(['$x->i16 - $x->ui8' => $x->i16 - $x->ui8, 'value' => $v]);
    var_dump(['$x->i16 / $x->ui8' => (int)($x->i16 / $x->ui8), 'value' => $v]);
    var_dump(['$x->i16 + $x->ui16' => $x->i16 + $x->ui16, 'value' => $v]);
    var_dump(['$x->i16 - $x->ui16' => $x->i16 - $x->ui16, 'value' => $v]);
    var_dump(['$x->i16 / $x->ui16' => (int)($x->i16 / $x->ui16), 'value' => $v]);
    var_dump(['$x->i16 + $x->ui32' => $x->i16 + $x->ui32, 'value' => $v]);
    var_dump(['$x->i16 - $x->ui32' => $x->i16 - $x->ui32, 'value' => $v]);
    var_dump(['$x->i16 / $x->ui32' => (int)($x->i16 / $x->ui32), 'value' => $v]);
    var_dump(['$x->i16 + $x->ui64' => $x->i16 + $x->ui64, 'value' => $v]);
    var_dump(['$x->i16 - $x->ui64' => $x->i16 - $x->ui64, 'value' => $v]);
    var_dump(['$x->i16 / $x->ui64' => (int)($x->i16 / $x->ui64), 'value' => $v]);
    var_dump(['$x->i32 + $x->i8' => $x->i32 + $x->i8, 'value' => $v]);
    var_dump(['$x->i32 - $x->i8' => $x->i32 - $x->i8, 'value' => $v]);
    var_dump(['$x->i32 / $x->i8' => (int)($x->i32 / $x->i8), 'value' => $v]);
    var_dump(['$x->i32 + $x->i16' => $x->i32 + $x->i16, 'value' => $v]);
    var_dump(['$x->i32 - $x->i16' => $x->i32 - $x->i16, 'value' => $v]);
    var_dump(['$x->i32 / $x->i16' => (int)($x->i32 / $x->i16), 'value' => $v]);
    var_dump(['$x->i32 + $x->i32' => $x->i32 + $x->i32, 'value' => $v]);
    var_dump(['$x->i32 - $x->i32' => $x->i32 - $x->i32, 'value' => $v]);
    var_dump(['$x->i32 / $x->i32' => (int)($x->i32 / $x->i32), 'value' => $v]);
    var_dump(['$x->i32 + $x->i64' => $x->i32 + $x->i64, 'value' => $v]);
    var_dump(['$x->i32 - $x->i64' => $x->i32 - $x->i64, 'value' => $v]);
    var_dump(['$x->i32 / $x->i64' => (int)($x->i32 / $x->i64), 'value' => $v]);
    var_dump(['$x->i32 + $x->ui8' => $x->i32 + $x->ui8, 'value' => $v]);
    var_dump(['$x->i32 - $x->ui8' => $x->i32 - $x->ui8, 'value' => $v]);
    var_dump(['$x->i32 / $x->ui8' => (int)($x->i32 / $x->ui8), 'value' => $v]);
    var_dump(['$x->i32 + $x->ui16' => $x->i32 + $x->ui16, 'value' => $v]);
    var_dump(['$x->i32 - $x->ui16' => $x->i32 - $x->ui16, 'value' => $v]);
    var_dump(['$x->i32 / $x->ui16' => (int)($x->i32 / $x->ui16), 'value' => $v]);
    var_dump(['$x->i32 + $x->ui32' => $x->i32 + $x->ui32, 'value' => $v]);
    var_dump(['$x->i32 - $x->ui32' => $x->i32 - $x->ui32, 'value' => $v]);
    var_dump(['$x->i32 / $x->ui32' => (int)($x->i32 / $x->ui32), 'value' => $v]);
    var_dump(['$x->i32 + $x->ui64' => $x->i32 + $x->ui64, 'value' => $v]);
    var_dump(['$x->i32 - $x->ui64' => $x->i32 - $x->ui64, 'value' => $v]);
    var_dump(['$x->i32 / $x->ui64' => (int)($x->i32 / $x->ui64), 'value' => $v]);
    var_dump(['$x->i64 + $x->i8' => $x->i64 + $x->i8, 'value' => $v]);
    var_dump(['$x->i64 - $x->i8' => $x->i64 - $x->i8, 'value' => $v]);
    var_dump(['$x->i64 / $x->i8' => (int)($x->i64 / $x->i8), 'value' => $v]);
    var_dump(['$x->i64 + $x->i16' => $x->i64 + $x->i16, 'value' => $v]);
    var_dump(['$x->i64 - $x->i16' => $x->i64 - $x->i16, 'value' => $v]);
    var_dump(['$x->i64 / $x->i16' => (int)($x->i64 / $x->i16), 'value' => $v]);
    var_dump(['$x->i64 + $x->i32' => $x->i64 + $x->i32, 'value' => $v]);
    var_dump(['$x->i64 - $x->i32' => $x->i64 - $x->i32, 'value' => $v]);
    var_dump(['$x->i64 / $x->i32' => (int)($x->i64 / $x->i32), 'value' => $v]);
    var_dump(['$x->i64 + $x->i64' => $x->i64 + $x->i64, 'value' => $v]);
    var_dump(['$x->i64 - $x->i64' => $x->i64 - $x->i64, 'value' => $v]);
    var_dump(['$x->i64 / $x->i64' => (int)($x->i64 / $x->i64), 'value' => $v]);
    var_dump(['$x->i64 + $x->ui8' => $x->i64 + $x->ui8, 'value' => $v]);
    var_dump(['$x->i64 - $x->ui8' => $x->i64 - $x->ui8, 'value' => $v]);
    var_dump(['$x->i64 / $x->ui8' => (int)($x->i64 / $x->ui8), 'value' => $v]);
    var_dump(['$x->i64 + $x->ui16' => $x->i64 + $x->ui16, 'value' => $v]);
    var_dump(['$x->i64 - $x->ui16' => $x->i64 - $x->ui16, 'value' => $v]);
    var_dump(['$x->i64 / $x->ui16' => (int)($x->i64 / $x->ui16), 'value' => $v]);
    var_dump(['$x->i64 + $x->ui32' => $x->i64 + $x->ui32, 'value' => $v]);
    var_dump(['$x->i64 - $x->ui32' => $x->i64 - $x->ui32, 'value' => $v]);
    var_dump(['$x->i64 / $x->ui32' => (int)($x->i64 / $x->ui32), 'value' => $v]);
    var_dump(['$x->i64 + $x->ui64' => $x->i64 + $x->ui64, 'value' => $v]);
    var_dump(['$x->i64 - $x->ui64' => $x->i64 - $x->ui64, 'value' => $v]);
    var_dump(['$x->i64 / $x->ui64' => (int)($x->i64 / $x->ui64), 'value' => $v]);
    var_dump(['$x->ui8 + $x->i8' => $x->ui8 + $x->i8, 'value' => $v]);
    var_dump(['$x->ui8 - $x->i8' => $x->ui8 - $x->i8, 'value' => $v]);
    var_dump(['$x->ui8 / $x->i8' => (int)($x->ui8 / $x->i8), 'value' => $v]);
    var_dump(['$x->ui8 + $x->i16' => $x->ui8 + $x->i16, 'value' => $v]);
    var_dump(['$x->ui8 - $x->i16' => $x->ui8 - $x->i16, 'value' => $v]);
    var_dump(['$x->ui8 / $x->i16' => (int)($x->ui8 / $x->i16), 'value' => $v]);
    var_dump(['$x->ui8 + $x->i32' => $x->ui8 + $x->i32, 'value' => $v]);
    var_dump(['$x->ui8 - $x->i32' => $x->ui8 - $x->i32, 'value' => $v]);
    var_dump(['$x->ui8 / $x->i32' => (int)($x->ui8 / $x->i32), 'value' => $v]);
    var_dump(['$x->ui8 + $x->i64' => $x->ui8 + $x->i64, 'value' => $v]);
    var_dump(['$x->ui8 - $x->i64' => $x->ui8 - $x->i64, 'value' => $v]);
    var_dump(['$x->ui8 / $x->i64' => (int)($x->ui8 / $x->i64), 'value' => $v]);
    var_dump(['$x->ui8 + $x->ui8' => $x->ui8 + $x->ui8, 'value' => $v]);
    var_dump(['$x->ui8 - $x->ui8' => $x->ui8 - $x->ui8, 'value' => $v]);
    var_dump(['$x->ui8 / $x->ui8' => (int)($x->ui8 / $x->ui8), 'value' => $v]);
    var_dump(['$x->ui8 + $x->ui16' => $x->ui8 + $x->ui16, 'value' => $v]);
    var_dump(['$x->ui8 - $x->ui16' => $x->ui8 - $x->ui16, 'value' => $v]);
    var_dump(['$x->ui8 / $x->ui16' => (int)($x->ui8 / $x->ui16), 'value' => $v]);
    var_dump(['$x->ui8 + $x->ui32' => $x->ui8 + $x->ui32, 'value' => $v]);
    var_dump(['$x->ui8 - $x->ui32' => $x->ui8 - $x->ui32, 'value' => $v]);
    var_dump(['$x->ui8 / $x->ui32' => (int)($x->ui8 / $x->ui32), 'value' => $v]);
    var_dump(['$x->ui8 + $x->ui64' => $x->ui8 + $x->ui64, 'value' => $v]);
    var_dump(['$x->ui8 - $x->ui64' => $x->ui8 - $x->ui64, 'value' => $v]);
    var_dump(['$x->ui8 / $x->ui64' => (int)($x->ui8 / $x->ui64), 'value' => $v]);
    var_dump(['$x->ui16 + $x->i8' => $x->ui16 + $x->i8, 'value' => $v]);
    var_dump(['$x->ui16 - $x->i8' => $x->ui16 - $x->i8, 'value' => $v]);
    var_dump(['$x->ui16 / $x->i8' => (int)($x->ui16 / $x->i8), 'value' => $v]);
    var_dump(['$x->ui16 + $x->i16' => $x->ui16 + $x->i16, 'value' => $v]);
    var_dump(['$x->ui16 - $x->i16' => $x->ui16 - $x->i16, 'value' => $v]);
    var_dump(['$x->ui16 / $x->i16' => (int)($x->ui16 / $x->i16), 'value' => $v]);
    var_dump(['$x->ui16 + $x->i32' => $x->ui16 + $x->i32, 'value' => $v]);
    var_dump(['$x->ui16 - $x->i32' => $x->ui16 - $x->i32, 'value' => $v]);
    var_dump(['$x->ui16 / $x->i32' => (int)($x->ui16 / $x->i32), 'value' => $v]);
    var_dump(['$x->ui16 + $x->i64' => $x->ui16 + $x->i64, 'value' => $v]);
    var_dump(['$x->ui16 - $x->i64' => $x->ui16 - $x->i64, 'value' => $v]);
    var_dump(['$x->ui16 / $x->i64' => (int)($x->ui16 / $x->i64), 'value' => $v]);
    var_dump(['$x->ui16 + $x->ui8' => $x->ui16 + $x->ui8, 'value' => $v]);
    var_dump(['$x->ui16 - $x->ui8' => $x->ui16 - $x->ui8, 'value' => $v]);
    var_dump(['$x->ui16 / $x->ui8' => (int)($x->ui16 / $x->ui8), 'value' => $v]);
    var_dump(['$x->ui16 + $x->ui16' => $x->ui16 + $x->ui16, 'value' => $v]);
    var_dump(['$x->ui16 - $x->ui16' => $x->ui16 - $x->ui16, 'value' => $v]);
    var_dump(['$x->ui16 / $x->ui16' => (int)($x->ui16 / $x->ui16), 'value' => $v]);
    var_dump(['$x->ui16 + $x->ui32' => $x->ui16 + $x->ui32, 'value' => $v]);
    var_dump(['$x->ui16 - $x->ui32' => $x->ui16 - $x->ui32, 'value' => $v]);
    var_dump(['$x->ui16 / $x->ui32' => (int)($x->ui16 / $x->ui32), 'value' => $v]);
    var_dump(['$x->ui16 + $x->ui64' => $x->ui16 + $x->ui64, 'value' => $v]);
    var_dump(['$x->ui16 - $x->ui64' => $x->ui16 - $x->ui64, 'value' => $v]);
    var_dump(['$x->ui16 / $x->ui64' => (int)($x->ui16 / $x->ui64), 'value' => $v]);
    var_dump(['$x->ui32 + $x->i8' => $x->ui32 + $x->i8, 'value' => $v]);
    var_dump(['$x->ui32 - $x->i8' => $x->ui32 - $x->i8, 'value' => $v]);
    var_dump(['$x->ui32 / $x->i8' => (int)($x->ui32 / $x->i8), 'value' => $v]);
    var_dump(['$x->ui32 + $x->i16' => $x->ui32 + $x->i16, 'value' => $v]);
    var_dump(['$x->ui32 - $x->i16' => $x->ui32 - $x->i16, 'value' => $v]);
    var_dump(['$x->ui32 / $x->i16' => (int)($x->ui32 / $x->i16), 'value' => $v]);
    var_dump(['$x->ui32 + $x->i32' => $x->ui32 + $x->i32, 'value' => $v]);
    var_dump(['$x->ui32 - $x->i32' => $x->ui32 - $x->i32, 'value' => $v]);
    var_dump(['$x->ui32 / $x->i32' => (int)($x->ui32 / $x->i32), 'value' => $v]);
    var_dump(['$x->ui32 + $x->i64' => $x->ui32 + $x->i64, 'value' => $v]);
    var_dump(['$x->ui32 - $x->i64' => $x->ui32 - $x->i64, 'value' => $v]);
    var_dump(['$x->ui32 / $x->i64' => (int)($x->ui32 / $x->i64), 'value' => $v]);
    var_dump(['$x->ui32 + $x->ui8' => $x->ui32 + $x->ui8, 'value' => $v]);
    var_dump(['$x->ui32 - $x->ui8' => $x->ui32 - $x->ui8, 'value' => $v]);
    var_dump(['$x->ui32 / $x->ui8' => (int)($x->ui32 / $x->ui8), 'value' => $v]);
    var_dump(['$x->ui32 + $x->ui16' => $x->ui32 + $x->ui16, 'value' => $v]);
    var_dump(['$x->ui32 - $x->ui16' => $x->ui32 - $x->ui16, 'value' => $v]);
    var_dump(['$x->ui32 / $x->ui16' => (int)($x->ui32 / $x->ui16), 'value' => $v]);
    var_dump(['$x->ui32 + $x->ui32' => $x->ui32 + $x->ui32, 'value' => $v]);
    var_dump(['$x->ui32 - $x->ui32' => $x->ui32 - $x->ui32, 'value' => $v]);
    var_dump(['$x->ui32 / $x->ui32' => (int)($x->ui32 / $x->ui32), 'value' => $v]);
    var_dump(['$x->ui32 + $x->ui64' => $x->ui32 + $x->ui64, 'value' => $v]);
    var_dump(['$x->ui32 - $x->ui64' => $x->ui32 - $x->ui64, 'value' => $v]);
    var_dump(['$x->ui32 / $x->ui64' => (int)($x->ui32 / $x->ui64), 'value' => $v]);
    var_dump(['$x->ui64 + $x->i8' => $x->ui64 + $x->i8, 'value' => $v]);
    var_dump(['$x->ui64 - $x->i8' => $x->ui64 - $x->i8, 'value' => $v]);
    var_dump(['$x->ui64 / $x->i8' => (int)($x->ui64 / $x->i8), 'value' => $v]);
    var_dump(['$x->ui64 + $x->i16' => $x->ui64 + $x->i16, 'value' => $v]);
    var_dump(['$x->ui64 - $x->i16' => $x->ui64 - $x->i16, 'value' => $v]);
    var_dump(['$x->ui64 / $x->i16' => (int)($x->ui64 / $x->i16), 'value' => $v]);
    var_dump(['$x->ui64 + $x->i32' => $x->ui64 + $x->i32, 'value' => $v]);
    var_dump(['$x->ui64 - $x->i32' => $x->ui64 - $x->i32, 'value' => $v]);
    var_dump(['$x->ui64 / $x->i32' => (int)($x->ui64 / $x->i32), 'value' => $v]);
    var_dump(['$x->ui64 + $x->i64' => $x->ui64 + $x->i64, 'value' => $v]);
    var_dump(['$x->ui64 - $x->i64' => $x->ui64 - $x->i64, 'value' => $v]);
    var_dump(['$x->ui64 / $x->i64' => (int)($x->ui64 / $x->i64), 'value' => $v]);
    var_dump(['$x->ui64 + $x->ui8' => $x->ui64 + $x->ui8, 'value' => $v]);
    var_dump(['$x->ui64 - $x->ui8' => $x->ui64 - $x->ui8, 'value' => $v]);
    var_dump(['$x->ui64 / $x->ui8' => (int)($x->ui64 / $x->ui8), 'value' => $v]);
    var_dump(['$x->ui64 + $x->ui16' => $x->ui64 + $x->ui16, 'value' => $v]);
    var_dump(['$x->ui64 - $x->ui16' => $x->ui64 - $x->ui16, 'value' => $v]);
    var_dump(['$x->ui64 / $x->ui16' => (int)($x->ui64 / $x->ui16), 'value' => $v]);
    var_dump(['$x->ui64 + $x->ui32' => $x->ui64 + $x->ui32, 'value' => $v]);
    var_dump(['$x->ui64 - $x->ui32' => $x->ui64 - $x->ui32, 'value' => $v]);
    var_dump(['$x->ui64 / $x->ui32' => (int)($x->ui64 / $x->ui32), 'value' => $v]);
    var_dump(['$x->ui64 + $x->ui64' => $x->ui64 + $x->ui64, 'value' => $v]);
    var_dump(['$x->ui64 - $x->ui64' => $x->ui64 - $x->ui64, 'value' => $v]);
    var_dump(['$x->ui64 / $x->ui64' => (int)($x->ui64 / $x->ui64), 'value' => $v]);
  }
}

test();
