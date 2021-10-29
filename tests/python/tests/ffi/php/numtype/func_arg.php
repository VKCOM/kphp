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
  };
  ');
  $x = $cdef->new('struct NumTypes');

  ensure_int($x->i8);
  ensure_int($x->i16);
  ensure_int($x->i32);
  ensure_int($x->i64);

  ensure_int($x->ui8);
  ensure_int($x->ui16);
  ensure_int($x->ui32);
  ensure_int($x->ui64);

  ensure_float($x->f32);
  ensure_float($x->f64);
}

test();
