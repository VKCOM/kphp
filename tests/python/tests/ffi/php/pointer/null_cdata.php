<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_pointers_lib();

/** @return ?int */
function nullable_int(bool $cond) { return $cond ? 1 : null; }

/** @return ?bool */
function nullable_bool(bool $cond) { return $cond ? true : null; }

/** @return ?float */
function nullable_float(bool $cond) { return $cond ? 1.0 : null; }

function test_null_php2c() {
  $cdef = FFI::cdef('
    struct Types {
      int8_t i8;
      int16_t i16;
      int32_t i32;
      int64_t i64;
      uint8_t u8;
      uint16_t u16;
      uint32_t u32;
      uint64_t u64;
      bool b;
      float f32;
      double f64;
    };
  ');

  $types = $cdef->new('struct Types');

  $types->i8 = nullable_int(true);
  var_dump($types->i8);
  $types->i8 = nullable_int(false);
  var_dump($types->i8);

  $types->i16 = nullable_int(true);
  var_dump($types->i16);
  $types->i16 = nullable_int(false);
  var_dump($types->i16);

  $types->i32 = nullable_int(true);
  var_dump($types->i32);
  $types->i32 = nullable_int(false);
  var_dump($types->i32);

  $types->i64 = nullable_int(true);
  var_dump($types->i64);
  $types->i64 = nullable_int(false);
  var_dump($types->i64);

  $types->u8 = nullable_int(true);
  var_dump($types->u8);
  $types->u8 = nullable_int(false);
  var_dump($types->u8);

  $types->u16 = nullable_int(true);
  var_dump($types->u16);
  $types->u16 = nullable_int(false);
  var_dump($types->u16);

  $types->u32 = nullable_int(true);
  var_dump($types->u32);
  $types->u32 = nullable_int(false);
  var_dump($types->u32);

  $types->u64 = nullable_int(true);
  var_dump($types->u64);
  $types->u64 = nullable_int(false);
  var_dump($types->u64);

  $types->b = nullable_bool(true);
  var_dump($types->b);
  $types->b = nullable_bool(false);
  var_dump($types->b);

  $types->f32 = nullable_float(true);
  var_dump($types->f32);
  $types->f32 = nullable_float(false);
  var_dump($types->f32);

  $types->f64 = nullable_float(true);
  var_dump($types->f64);
  $types->f64 = nullable_float(false);
  var_dump($types->f64);
}

$lib = \FFI::scope('pointers');

$result = $lib->nullptr_data();
var_dump($result === null);
var_dump($result !== null);

$intptr = \FFI::new('int*');
var_dump($intptr === null);
var_dump($intptr !== null);

test_null_php2c();
