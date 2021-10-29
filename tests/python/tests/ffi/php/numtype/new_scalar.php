<?php

function test_int64_t(int $value) {
  $x = FFI::new('int64_t');

  var_dump($x->cdata);
  $x->cdata = $value;
  var_dump($x->cdata);

  $unboxed = $x->cdata;
  var_dump($unboxed);

  $y = $x;
  var_dump($y->cdata);
  $y->cdata = $x->cdata;
  var_dump($y->cdata);
}

function test_int32_t(int $value) {
  $x = FFI::new('int32_t');

  var_dump($x->cdata);
  $x->cdata = $value;
  var_dump($x->cdata);

  $unboxed = $x->cdata;
  var_dump($unboxed);

  $y = $x;
  var_dump($y->cdata);
  $y->cdata = $x->cdata;
  var_dump($y->cdata);
}

function test_int16_t(int $value) {
  $x = FFI::new('int16_t');

  var_dump($x->cdata);
  $x->cdata = $value;
  var_dump($x->cdata);

  $unboxed = $x->cdata;
  var_dump($unboxed);

  $y = $x;
  var_dump($y->cdata);
  $y->cdata = $x->cdata;
  var_dump($y->cdata);
}

function test_int8_t(int $value) {
  $x = FFI::new('int8_t');

  var_dump($x->cdata);
  $x->cdata = $value;
  var_dump($x->cdata);

  $unboxed = $x->cdata;
  var_dump($unboxed);

  $y = $x;
  var_dump($y->cdata);
  $y->cdata = $x->cdata;
  var_dump($y->cdata);
}

function test_uint64_t(int $value) {
  $x = FFI::new('uint64_t');

  var_dump($x->cdata);
  $x->cdata = $value;
  var_dump($x->cdata);

  $unboxed = $x->cdata;
  var_dump($unboxed);

  $y = $x;
  var_dump($y->cdata);
  $y->cdata = $x->cdata;
  var_dump($y->cdata);
}

function test_uint32_t(int $value) {
  $x = FFI::new('uint32_t');

  var_dump($x->cdata);
  $x->cdata = $value;
  var_dump($x->cdata);

  $unboxed = $x->cdata;
  var_dump($unboxed);

  $y = $x;
  var_dump($y->cdata);
  $y->cdata = $x->cdata;
  var_dump($y->cdata);
}

function test_uint16_t(int $value) {
  $x = FFI::new('uint16_t');

  var_dump($x->cdata);
  $x->cdata = $value;
  var_dump($x->cdata);

  $unboxed = $x->cdata;
  var_dump($unboxed);

  $y = $x;
  var_dump($y->cdata);
  $y->cdata = $x->cdata;
  var_dump($y->cdata);
}

function test_uint8_t(int $value) {
  $x = FFI::new('uint8_t');

  var_dump($x->cdata);
  $x->cdata = $value;
  var_dump($x->cdata);

  $unboxed = $x->cdata;
  var_dump($unboxed);

  $y = $x;
  var_dump($y->cdata);
  $y->cdata = $x->cdata;
  var_dump($y->cdata);
}

function test() {
  $int_values = [-5392, -100, 0, 1, 254, 255, 0xffff, 1498884825];
  foreach ($int_values as $value) {
    test_int8_t($value);
//     test_int16_t($value);
//     test_int32_t($value);
//     test_int64_t($value);
//     test_uint8_t($value);
//     test_uint16_t($value);
//     test_uint32_t($value);
//     test_uint64_t($value);
  }
}

test();
