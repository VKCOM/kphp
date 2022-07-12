<?php

require_once __DIR__ . '/include/common.php';

kphp_load_pointers_lib();

function test_assert(int $line, bool $v) {
  if (!$v) {
    echo "assertion failed as line $line\n";
  }
}

/**
 * @param ffi_cdata<C, uint32_t[]> $arr
 */
function fill_uint32_array($arr, int $value) {
  for ($i = 0; $i < count($arr); $i++) {
    ffi_array_set($arr, $i, $value);
  }
}

/**
 * @param ffi_cdata<C, uint32_t[10]> $arr
 */
function fill_uint32_array10($arr, int $value) {
  fill_uint32_array($arr, $value);
}

function test_uint32_array() {
  $arr15 = FFI::new('uint32_t[15]');
  test_assert(__LINE__, FFI::sizeof($arr15) === (15 * 4));
  test_assert(__LINE__, count($arr15) === 15);

  $arr10 = FFI::new('uint32_t[10]');
  test_assert(__LINE__, FFI::sizeof($arr10) === (10 * 4));
  test_assert(__LINE__, count($arr10) === 10);

  // can pass fixed size arrays as T[] params freely
  fill_uint32_array($arr15, 11);
  for ($i = 0; $i < count($arr15); $i++) {
    test_assert(__LINE__, ffi_array_get($arr15, $i) === 11);
  }
  fill_uint32_array($arr10, 22);
  for ($i = 0; $i < count($arr10); $i++) {
    test_assert(__LINE__, ffi_array_get($arr10, $i) === 22);
  }

  $arr15_b = $arr15; // no array copying happens here
  ffi_array_set($arr15_b, 0, 100); // this modification is seen in both array vars
  test_assert(__LINE__, ffi_array_get($arr15, 0) === 100);
  test_assert(__LINE__, ffi_array_get($arr15_b, 0) === 100);

  fill_uint32_array10($arr10, 111);
  test_assert(__LINE__, ffi_array_get($arr10, 0) === 111);
  test_assert(__LINE__, ffi_array_get($arr10, count($arr10)-1) === 111);
}

/**
 * @param ffi_scope<example> $example_lib
 */
function test_struct_array($example_lib) {
  $vectors = $example_lib->new('struct Vec2i[5]');
  $sizeof_vector = FFI::sizeof(ffi_array_get($vectors, 0));
  test_assert(__LINE__, FFI::sizeof($vectors) === (count($vectors) * $sizeof_vector));

  test_assert(__LINE__, ffi_array_get($vectors, 0)->x === 0);
  ffi_array_get($vectors, 0)->x = 10;
  test_assert(__LINE__, ffi_array_get($vectors, 0)->x === 10);
  $vec = $example_lib->new('struct Vec2i');
  $vec->x = 111;
  $vec->y = 231;
  ffi_array_set($vectors, 0, $vec);
  test_assert(__LINE__, ffi_array_get($vectors, 0)->x === $vec->x);
  test_assert(__LINE__, ffi_array_get($vectors, 0)->y === $vec->y);
  $vec->x = -14;
  test_assert(__LINE__, ffi_array_get($vectors, 0)->x !== $vec->x);
  ffi_array_get($vectors, 0)->x = 28;
  test_assert(__LINE__, ffi_array_get($vectors, 0)->x === 28);
  test_assert(__LINE__, ffi_array_get($vectors, 0)->x !== $vec->x);

  $vec_ref = ffi_array_get($vectors, 1);
  $vec_ref->x = 103;
  $vec_ref->y = -2491;
  test_assert(__LINE__, $vec_ref->x === 103);
  test_assert(__LINE__, $vec_ref->y === -2491);
  test_assert(__LINE__, ffi_array_get($vectors, 1)->x === $vec_ref->x);
  test_assert(__LINE__, ffi_array_get($vectors, 1)->y === $vec_ref->y);

  $arr2 = $example_lib->new('struct Vec2i[2]');
  ffi_array_set($arr2, 0, clone ffi_array_get($vectors, 1));
  test_assert(__LINE__, ffi_array_get($vectors, 1)->x === ffi_array_get($arr2, 0)->x);
  test_assert(__LINE__, ffi_array_get($vectors, 1)->y === ffi_array_get($arr2, 0)->y);
}

/**
 * @param ffi_scope<example> $example_lib
 */
function test_struct_field_array($example_lib) {
  $mem32 = $example_lib->new('struct Mem32');
  for ($i = 0; $i < 4; $i++) {
    $v = ($i+1) * 10;
    ffi_array_set($mem32->data, $i, $v);
    test_assert(__LINE__, ffi_array_get($mem32->data, $i) === $v);
  }
}

$example_lib = FFI::cdef('
#define FFI_SCOPE "example"

struct Mem32 {
  uint8_t data[32];
};

struct Vec2i {
  int32_t x;
  int32_t y;
};

struct Line2D {
  struct Vec2i a;
  struct Vec2i b;
};

struct Line2DList {
  struct Line2D current;
  struct Line2D *next;
};
');

test_uint32_array();
test_struct_array($example_lib);
test_struct_field_array($example_lib);
