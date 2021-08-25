<?php

function ensure_int(int $x) { var_dump($x, gettype($x) === 'integer'); }
function ensure_float(float $x) { var_dump($x, gettype($x) === 'double'); }

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

    float f32;
    double f64;
  };');
  $x = $cdef->new('struct NumTypes');

  $int_array = [$x->i8, $x->i16, $x->i32, $x->i64];
  $int_array2 = [$x->ui8, $x->ui16, $x->ui32, $x->ui64];
  $float_array = [$x->f32, $x->f64];

  ensure_int($int_array[0]);
  ensure_int($int_array2[0]);
  ensure_float($float_array[0]);
}

test();
