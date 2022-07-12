<?php

require_once __DIR__ . '/../include/common.php';

function test() {
  $cdef = FFI::cdef('
    struct Foo {
      int8_t b0;
      int8_t b1;
      int8_t b2;
      int8_t b3;
    };
    struct Bar {
      uint32_t x;
      struct Foo foo;
    };
  ');
  $foo = $cdef->new('struct Foo');
  $bar = $cdef->new('struct Bar');
  $bar2 = $cdef->new('struct Bar');
  $int8 = $cdef->new('int8_t');
  $uint16 = $cdef->new('uint16_t');

  $int8->cdata = 10;
  FFI::memcpy($uint16, $int8, 1);
  var_dump($uint16->cdata);

  $uint16->cdata = 9121;
  FFI::memcpy($int8, $uint16, 1);
  var_dump($int8->cdata);

  $bar->x = 49249;
  FFI::memcpy($foo, $bar, 4);
  var_dump([$foo->b0, $foo->b1, $foo->b2, $foo->b3]);

  FFI::memcpy($bar2, $bar, FFI::sizeof($bar));
  var_dump([$bar2->x, $bar2->foo->b0]);

  $bar->x = 237473249312;
  FFI::memcpy(FFI::addr($foo), $bar, 3);
  var_dump([$foo->b0, $foo->b1, $foo->b2, $foo->b3]);

  $bar->x = 0;
  FFI::memcpy($foo, FFI::addr($bar), 1);
  var_dump([$foo->b0, $foo->b1, $foo->b2, $foo->b3]);

  $bar->x = 0xffff;
  FFI::memcpy(FFI::addr($foo), FFI::addr($bar), 2);
  var_dump([$foo->b0, $foo->b1, $foo->b2, $foo->b3]);

  var_dump($bar->x);
  FFI::memcpy($bar, $foo, 4);
  var_dump($bar->x);

  $bar->x = 0;
  FFI::memcpy(FFI::addr($bar), $foo, 1);
  var_dump($bar->x);
}

function test_arrays() {
  $fixed_array = FFI::new('int32_t[10]');
  for ($i = 0; $i < count($fixed_array); $i++) {
    ffi_array_set($fixed_array, $i, $i*2);
  }
  $fixed_array2 = FFI::new('int32_t[10]');
  FFI::memcpy($fixed_array2, $fixed_array, FFI::sizeof($fixed_array));
  for ($i = 0; $i < count($fixed_array2); $i++) {
    var_dump(ffi_array_get($fixed_array2, $i));
  }

  $arr_len = 17;
  $dynamic_array = FFI::new("int32_t[$arr_len]");
  for ($i = 0; $i < count($dynamic_array); $i++) {
    ffi_array_set($dynamic_array, $i, $i*4+10);
  }
  $dynamic_array2 = FFI::new('int32_t[' . (int)($arr_len / 2) . ']');
  FFI::memcpy($dynamic_array2, $dynamic_array, FFI::sizeof($dynamic_array2));
  for ($i = 0; $i < count($dynamic_array2); $i++) {
    var_dump(ffi_array_get($dynamic_array2, $i));
  }

  FFI::memcpy($fixed_array, $dynamic_array, FFI::sizeof($fixed_array));
  for ($i = 0; $i < count($fixed_array); $i++) {
    var_dump(ffi_array_get($fixed_array, $i));
  }

  $cdef = FFI::cdef('
    struct A {
      int64_t ints[10];
    };
    struct B {
      struct A *a;
    };
  ');
  $a = $cdef->new('struct A');
  for ($i = 0; $i < count($a->ints); $i++) {
    ffi_array_set($a->ints, $i, $i << 2);
    var_dump(ffi_array_get($a->ints, $i));
  }
  $a_clone = clone $a;
  ffi_array_set($a_clone->ints, 0, -13);
  var_dump(ffi_array_get($a->ints, 0) !== ffi_array_get($a_clone->ints, 0));
  var_dump(ffi_array_get($a->ints, 1) === ffi_array_get($a_clone->ints, 1));
  $b = $cdef->new('struct B');
  $b->a = FFI::addr($a);
  var_dump(ffi_array_get($b->a->ints, 0) !== ffi_array_get($a_clone->ints, 0));
  var_dump(ffi_array_get($b->a->ints, 1) === ffi_array_get($a_clone->ints, 1));
  FFI::memcpy($b->a->ints, $a_clone->ints, FFI::sizeof($b->a->ints));
  for ($i = 0; $i < count($b->a->ints); $i++) {
    var_dump(ffi_array_get($b->a->ints, $i));
    var_dump(ffi_array_get($a->ints, $i));
    var_dump(ffi_array_get($a_clone->ints, $i));
    ffi_array_set($b->a->ints, $i, -$i - 10);
  }
  FFI::memcpy($a_clone->ints, $b->a->ints, FFI::sizeof($a_clone->ints));
  for ($i = 0; $i < count($a_clone->ints); $i++) {
    var_dump(ffi_array_get($b->a->ints, $i));
    var_dump(ffi_array_get($a->ints, $i));
    var_dump(ffi_array_get($a_clone->ints, $i));
  }
}

/**
 * @return ffi_cdata<C, char[]>
 * Note: it returns an array, not a pointer, so the returned cdata
 * retains the ownership of the memory (char* would be non-owning)
 */
function clone_string(string $s) {
  $len_with_nullbyte = strlen($s) + 1;
  $c_str = \FFI::new("char[$len_with_nullbyte]");
  ffi_memcpy_string($c_str, $s, strlen($s));
  return $c_str;
}

/** @return ffi_cdata<C, char[]> */
function test_strings() {
  $str = 'hello';
  $c_str = clone_string($str);
  for ($i = 0; $i < strlen($str); $i++) {
    var_dump(ffi_array_get($c_str, $i));
  }
  var_dump(\FFI::sizeof($c_str) === (strlen('hello') + 1));
  var_dump(\FFI::string($c_str) === 'hello');

  $empty_string = '';
  $c_empty_string = clone_string($empty_string);

  $c_str_array = \FFI::new('const char*[3]');
  ffi_array_set($c_str_array, 0, FFI::cast('const char*', $c_str));
  ffi_array_set($c_str_array, 1, FFI::cast('const char*', $c_empty_string));
  ffi_array_set($c_str_array, 2, null);

  var_dump(ffi_array_get($c_str_array, 0));
  var_dump(ffi_array_get($c_str_array, 1));
  var_dump(ffi_array_get($c_str_array, 2));
  var_dump(ffi_array_get($c_str_array, 2) === null);

  return $c_str;
}

test();
test_arrays();
$c_str = test_strings();
var_dump(\FFI::sizeof($c_str));
var_dump(\FFI::string($c_str));
