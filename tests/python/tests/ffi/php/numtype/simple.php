<?php

// This test checks simple php2c and c2php conversions for numeric types.

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

  $x->i8 = 8; // rhs: php2c(8)
  $i8 = $x->i8; // rhs: c2php($x->i8)
  $x->i8 = $x->i8; // rhs: no conversions
  var_dump($i8, gettype($i8) === 'integer');

  $x->i16 = 16;
  $i16 = $x->i16;
  $x->i16 = $x->i16;
  var_dump($i16, gettype($i16) === 'integer');

  $x->i32 = 32;
  $i32 = $x->i32;
  $x->i32 = $x->i32;
  var_dump($i32, gettype($i32) === 'integer');

  $x->i64 = 64;
  $i64 = $x->i64;
  $x->i64 = $x->i64;
  var_dump($i64, gettype($i64) === 'integer');

  $x->ui8 = 108;
  $ui8 = $x->ui8;
  $x->ui8 = $x->ui8;
  var_dump($ui8, gettype($ui8) === 'integer');

  $x->ui16 = 116;
  $ui16 = $x->ui16;
  $x->ui16 = $x->ui16;
  var_dump($ui16, gettype($ui16) === 'integer');

  $x->ui32 = 132;
  $ui32 = $x->ui32;
  $x->ui32 = $x->ui32;
  var_dump($ui32, gettype($ui32) === 'integer');

  $x->ui64 = 164;
  $ui64 = $x->ui64;
  $x->ui64 = $x->ui64;
  var_dump($ui64, gettype($ui64) === 'integer');

  $x->f32 = 45.5;
  $f32 = $x->f32;
  $x->f32 = $x->f32;
  var_dump($f32, gettype($f32) === 'double');

  $x->f64 = 3850.9;
  $f64 = $x->f64;
  $x->f64 = $x->f64;
  var_dump($f64, gettype($f64) === 'double');
}

test();
