<?php

require_once __DIR__ . '/include/common.php';

kphp_load_pointers_lib();

function test_assert(int $line, bool $v) {
  if (!$v) {
    echo "assertion failed at line $line\n";
  }
}

/**
 * @param ffi_cdata<C, uint32_t[]> $arr
 */
function uint32_array_sum($arr) {
  $result = 0;
  for ($i = 0; $i < count($arr); $i++) {
    $result += ffi_array_get($arr, $i);
  }
  return $result;
}

/**
 * @param ffi_cdata<example, struct Vec2i[]> $arr
 * @param int[][] $data
 */
function vec2i_array_data_eq($arr, $data): bool {
  if (count($arr) !== count($data)) {
    return false;
  }
  for ($i = 0; $i < count($arr); $i++) {
    if (ffi_array_get($arr, $i)->x !== $data[$i][0]) {
      return false;
    }
    if (ffi_array_get($arr, $i)->y !== $data[$i][1]) {
      return false;
    }
  }
  return true;
}

/**
 * @param ffi_scope<example> $lib
 * @param int[][] $points
 * @return ffi_cdata<example, struct Vec2i[]>
 */
function make_vec2_array_a($lib, array $points) {
  $c_array = $lib->new('struct Vec2i[' . count($points) . ']');
  // setting elements via $a[$i]->field
  foreach ($points as $i => $pair) {
    ffi_array_get($c_array, (int)$i)->x = (int)$pair[0];
    ffi_array_get($c_array, (int)$i)->y = (int)$pair[1];
  }
  return $c_array;
}

/**
 * @param ffi_scope<example> $lib
 * @param int[][] $points
 * @return ffi_cdata<example, struct Vec2i[]>
 */
function make_vec2_array_b($lib, array $points) {
  $c_array = $lib->new('struct Vec2i[' . count($points) . ']');
  // setting elements via struct assignment
  foreach ($points as $i => $pair) {
    $vec = $lib->new('struct Vec2i');
    $vec->x = (int)$pair[0];
    $vec->y = (int)$pair[1];
    ffi_array_set($c_array, (int)$i, $vec);
  }
  return $c_array;
}

/**
 * @param ffi_scope<example> $lib
 * @param int[][] $points
 * @return ffi_cdata<example, struct Vec2i[]>
 */
function make_vec2_array_c($lib, array $points) {
  $c_array = $lib->new('struct Vec2i[' . count($points) . ']');
  // setting elements via reference mutation
  foreach ($points as $i => $pair) {
    $ref = ffi_array_get($c_array, (int)$i);
    $ref->x = (int)$pair[0];
    $ref->y = (int)$pair[1];
  }
  return $c_array;
}

/**
 * @param ffi_scope<example> $lib
 * @param int[][] $points
 * @return ffi_cdata<example, struct Vec2i[]>
 */
function make_vec2_array_d($lib, array $points) {
  $c_array = $lib->new('struct Vec2i[' . count($points) . ']');
  // setting elements via reference cloning
  foreach ($points as $i => $pair) {
    $ref = ffi_array_get($c_array, (int)$i);
    $ref->x = (int)$pair[0];
    $ref->y = (int)$pair[1];
    ffi_array_set($c_array, (int)$i, clone $ref);
  }
  return $c_array;
}

/**
 * @param ffi_scope<example> $lib
 * @param ffi_cdata<example, struct Vec2i[]> $arr
 */
function clone_vec2i_array_a($lib, $arr) {
  $copied = $lib->new('struct Vec2i[' . count($arr) . ']');
  for ($i = 0; $i < count($arr); $i++) {
    $v = ffi_array_get($arr, $i);
    ffi_array_set($copied, $i, clone $v);
  }
  return $copied;
}

/**
 * @param ffi_scope<example> $lib
 * @param ffi_cdata<example, struct Vec2i[]> $arr
 */
function clone_vec2i_array_b($lib, $arr) {
  $copied = $lib->new('struct Vec2i[' . count($arr) . ']');
  for ($i = 0; $i < count($arr); $i++) {
    ffi_array_set($copied, $i, clone ffi_array_get($arr, $i));
  }
  return $copied;
}

/**
 * @param ffi_scope<example> $lib
 * @param ffi_cdata<example, struct Vec2i[]> $arr
 */
function clone_vec2i_array_c($lib, $arr) {
  $copied = $lib->new('struct Vec2i[' . count($arr) . ']');
  for ($i = 0; $i < count($arr); $i++) {
    ffi_array_get($copied, $i)->x = ffi_array_get($arr, $i)->x;
    ffi_array_get($copied, $i)->y = ffi_array_get($arr, $i)->y;
  }
  return $copied;
}

function test_uint32_array() {
  $size = 4;
  // using string concat for the array size
  $arr = FFI::new('uint32_t[' . ($size*2) . ']');

  test_assert(__LINE__, count($arr) === ($size*2));
  test_assert(__LINE__, FFI::sizeof($arr) === ($size*2*4));

  // scalars returned as a result of FFI array indexing are
  // not references; re-assigning them does nothing to the array itself
  $x = ffi_array_get($arr, 0);
  var_dump($x);
  $x = 15;
  var_dump($x);
  var_dump(ffi_array_get($arr, 0));

  ffi_array_set($arr, 0, 10);
  test_assert(__LINE__, ffi_array_get($arr, 0) === 10);

  test_assert(__LINE__, ffi_array_get($arr, count($arr)-1) === 0);
  ffi_array_set($arr, count($arr)-1, 20);
  test_assert(__LINE__, ffi_array_get($arr, count($arr)-1) === 20);

  test_assert(__LINE__, uint32_array_sum($arr) === 30);
}

/**
 * @param ffi_scope<example> $example_lib
 */
function test_struct_array($example_lib) {
  $n = 10;
  // using string interpolation for the array size
  $vec2i_arr = $example_lib->new("struct Vec2i[$n]");

  test_assert(__LINE__, count($vec2i_arr) === $n);
  test_assert(__LINE__, FFI::sizeof($vec2i_arr) === ($n * 8));

  // struct values are returned by references in both PHP and KPHP
  // modifications to $v0 are visible inside the array
  $v0 = ffi_array_get($vec2i_arr, 0);
  test_assert(__LINE__, $v0->x === 0);
  test_assert(__LINE__, $v0->y === 0);
  $v0->x = 13;
  $v0->y = 19;
  test_assert(__LINE__, $v0->x === 13);
  test_assert(__LINE__, $v0->y === 19);
  test_assert(__LINE__, ffi_array_get($vec2i_arr, 0)->x === $v0->x);
  test_assert(__LINE__, ffi_array_get($vec2i_arr, 0)->y === $v0->y);
}

/**
 * @param ffi_scope<example> $example_lib
 */
function test_make_array($example_lib) {
  $arr1_data = [[1, 2], [3, 4]];
  $arr1 = make_vec2_array_a($example_lib, $arr1_data);
  $sizeof_vec2i = FFI::sizeof(ffi_array_get($arr1, 0));
  test_assert(__LINE__, count($arr1) === 2);
  test_assert(__LINE__, FFI::sizeof($arr1) === ($sizeof_vec2i * count($arr1)));
  test_assert(__LINE__, vec2i_array_data_eq($arr1, $arr1_data));
  test_assert(__LINE__, !vec2i_array_data_eq($arr1, [[0, 0], [0, 0]]));

  $arr2_data = [[5, 6], [7, 8], [-1, -2]];
  $arr2 = make_vec2_array_b($example_lib, $arr2_data);
  test_assert(__LINE__, count($arr2) === 3);
  test_assert(__LINE__, FFI::sizeof($arr2) === ($sizeof_vec2i * count($arr2)));
  test_assert(__LINE__, vec2i_array_data_eq($arr2, $arr2_data));
  test_assert(__LINE__, !vec2i_array_data_eq($arr2, [[0, 0], [0, 0], [0, 0]]));

  $arr3_data = [[10, 20], [-10, -20]];
  $arr3 = make_vec2_array_c($example_lib, $arr3_data);
  test_assert(__LINE__, count($arr3) === 2);
  test_assert(__LINE__, FFI::sizeof($arr3) === ($sizeof_vec2i * count($arr3)));
  test_assert(__LINE__, vec2i_array_data_eq($arr3, $arr3_data));
  test_assert(__LINE__, !vec2i_array_data_eq($arr3, [[0, 0], [0, 0]]));

  $arr4_data = [[-55, -555], [-3, -6]];
  $arr4 = make_vec2_array_d($example_lib, $arr4_data);
  test_assert(__LINE__, count($arr4) === 2);
  test_assert(__LINE__, FFI::sizeof($arr4) === ($sizeof_vec2i * count($arr4)));
  test_assert(__LINE__, vec2i_array_data_eq($arr4, $arr4_data));
  test_assert(__LINE__, !vec2i_array_data_eq($arr4, [[0, 0], [0, 0]]));

  $arr4_copy = clone_vec2i_array_a($example_lib, $arr4);
  test_assert(__LINE__, count($arr4_copy) === count($arr4));
  test_assert(__LINE__, vec2i_array_data_eq($arr4_copy, $arr4_data));
  test_assert(__LINE__, !vec2i_array_data_eq($arr4_copy, [[0, 0], [0, 0]]));
  test_assert(__LINE__, vec2i_array_data_eq(clone_vec2i_array_b($example_lib, $arr4_copy), $arr4_data));
  test_assert(__LINE__, vec2i_array_data_eq(clone_vec2i_array_c($example_lib, $arr4_copy), $arr4_data));
}

function test_array_as_ptr() {
  $lib = FFI::scope('pointers');

  $size = 10;
  $byte_array = FFI::new("uint8_t[$size]");
  ffi_array_set($byte_array, 0, 10);
  $byte_ptr = FFI::cast('uint8_t*', $byte_array);
  test_assert(__LINE__, $lib->bytes_array_get($byte_ptr, 0) === 10);
  $lib->bytes_array_set($byte_ptr, 0, 20);
  test_assert(__LINE__, $lib->bytes_array_get($byte_ptr, 0) === 20);
  test_assert(__LINE__, ffi_array_get($byte_array, 0) === 20);
}

/**
 * @param ffi_scope<example> $example_lib
 */
function test_array_of_pointers($example_lib) {
  $n = 3;
  $arr_of_lists = $example_lib->new("struct Line2DList*[$n]");
  $list1 = $example_lib->new('struct Line2DList');
  $list2 = $example_lib->new('struct Line2DList');
  ffi_array_set($arr_of_lists, 0, FFI::addr($list1));
  ffi_array_set($arr_of_lists, 1, FFI::addr($list2));
  $list1->current = $example_lib->new('struct Line2D');
  $list1->current->a->x = 10;
  $list1->current->a->y = 15;
  $list1->current->b->x = 10;
  $list1->current->b->y = 15;
  test_assert(__LINE__, ffi_array_get($arr_of_lists, 0)->current->a->x === $list1->current->a->x);
  test_assert(__LINE__, ffi_array_get($arr_of_lists, 0)->current->a->y === $list1->current->a->y);
  test_assert(__LINE__, ffi_array_get($arr_of_lists, 0)->current->b->x === $list1->current->b->x);
  test_assert(__LINE__, ffi_array_get($arr_of_lists, 0)->current->b->y === $list1->current->b->y);
  $next = $example_lib->new('struct Line2D');
  $next->a->x = 100;
  $next->a->y = 112;
  $list1->next = FFI::addr($next);
  test_assert(__LINE__, $next->a->x === $list1->next->a->x);
  test_assert(__LINE__, $next->a->x === ffi_array_get($arr_of_lists, 0)->next->a->x);
  $list2->current = $example_lib->new('struct Line2D');
  $vec = $example_lib->new('struct Vec2i');
  $vec->x = 99;
  $vec->y = -99;
  $line2d = $example_lib->new('struct Line2D');
  $line2d->a = $vec;
  $list2->current = $line2d;
  var_dump($vec->x);
  var_dump($line2d->a->x);
  var_dump($list2->current->a->x);
  var_dump(ffi_array_get($arr_of_lists, 1)->current->a->x);
  $vec->x = 1;
  var_dump($vec->x);
  var_dump($line2d->a->x);
  var_dump($list2->current->a->x);
  var_dump(ffi_array_get($arr_of_lists, 1)->current->a->x);
  $line2d->a->x = 2;
  var_dump($vec->x);
  var_dump($line2d->a->x);
  var_dump($list2->current->a->x);
  var_dump(ffi_array_get($arr_of_lists, 1)->current->a->x);
  $list2->current->a->x = 3;
  var_dump($vec->x);
  var_dump($line2d->a->x);
  var_dump($list2->current->a->x);
  var_dump(ffi_array_get($arr_of_lists, 1)->current->a->x);
  ffi_array_get($arr_of_lists, 1)->current->a->x = 4;
  var_dump($vec->x);
  var_dump($line2d->a->x);
  var_dump($list2->current->a->x);
  var_dump(ffi_array_get($arr_of_lists, 1)->current->a->x);
}

class ArrayWrapper {
  /** @var ffi_cdata<C, int32_t[]> */
  public $int32_array;

  /** @var ffi_cdata<C, int32_t[]>[] */
  public $int32_array2;
}

function test_array_wrapping() {
  $wrapper = new ArrayWrapper();

  $size = 10;
  $wrapper->int32_array = FFI::new("int32_t[$size]");
  var_dump(count($wrapper->int32_array));
  var_dump(ffi_array_get($wrapper->int32_array, 0));
  ffi_array_set($wrapper->int32_array, 0, 113);
  var_dump(ffi_array_get($wrapper->int32_array, 0));

  $wrapper->int32_array2 = [];
  $wrapper->int32_array2[] = $wrapper->int32_array;
  var_dump(ffi_array_get($wrapper->int32_array2[0], 0));
  ffi_array_set($wrapper->int32_array2[0], 0, 213);
  var_dump(ffi_array_get($wrapper->int32_array2[0], 0));
  var_dump(ffi_array_get($wrapper->int32_array, 0));
}

function test_float_double() {
  $size = 2;

  $floats = FFI::new("float[$size]");
  var_dump(\FFI::sizeof($floats));
  $doubles = FFI::new("double[$size]");
  var_dump(\FFI::sizeof($doubles));
}

$example_lib = FFI::cdef('
#define FFI_SCOPE "example"

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
test_make_array($example_lib);
test_array_as_ptr();
test_array_of_pointers($example_lib);
test_array_wrapping();
test_float_double();
