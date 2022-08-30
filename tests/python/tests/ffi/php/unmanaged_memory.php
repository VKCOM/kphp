<?php

require_once __DIR__ . '/include/common.php';

kphp_load_pointers_lib();

function test_new_free() {
  // a simple sanity test that new+free compiles and doesn't cause any run time errors

  $scalar = FFI::new('uint32_t', false);
  FFI::free($scalar);

  $fixed_arr = FFI::new('uint8_t[10]', false);
  FFI::free($fixed_arr);

  $size = 15;
  $dynamic_arr = FFI::new("uint8_t[$size]", false);
  FFI::free($dynamic_arr);

  $cdef = FFI::cdef('
    struct Example {
      uint16_t x;
      int64_t y;
      double z;
    };
  ');
  $struct = $cdef->new('struct Example', false);
  FFI::free($struct);
  for ($i = 0; $i < 5; $i++) {
    $struct2 = $cdef->new('struct Example', false);
    $struct2->x = $i + 1;
    $struct2_two = $struct2;
    var_dump($struct2->x);
    FFI::free($struct2);
  }

  $dynamic_struct_ptr_arr = $cdef->new("struct Example*[$size]", false);
  FFI::free($dynamic_struct_ptr_arr);

  // note: casting to void* requires FFI::addr(), otherwise PHP will crash
  $dynamic_arr2 = FFI::new("uint8_t[$size]", false);
  ffi_array_set($dynamic_arr2, 0, 14);
  $dynamic_arr2_ptr = FFI::cast('void*', FFI::addr($dynamic_arr2));
  $dynamic_arr2_ref = FFI::cast("uint8_t[$size]", $dynamic_arr2_ptr);
  var_dump(ffi_array_get($dynamic_arr2_ref, 0));
  FFI::free($dynamic_arr2);
}

function init_uint64() {
  $lib = FFI::scope('pointers');

  $v = $lib->new('uint64_t', false);
  $v->cdata = 24;
  var_dump($v->cdata);

  $lib->set_unmanaged_data(FFI::addr($v));
  $uv = FFI::cast('uint64_t*', $lib->get_unmanaged_data());
  var_dump(ffi_array_get($uv, 0));

  // after this function exits, $v reference counter should go to 0
}

function test_uint64() {
  $lib = FFI::scope('pointers');

  // since previous $v is deallocated, unmanaged_data() would return garbage
  // if new() is used with owned=true; here we try to force a new allocation
  // for another uint64_t that would occupy the previously allocated slot;
  // with owned=false there should be no issues
  $v = $lib->new('uint64_t');
  $v->cdata = 2321;
  var_dump($v->cdata);

  $uv = FFI::cast('uint64_t*', $lib->get_unmanaged_data());
  var_dump(ffi_array_get($uv, 0));

  FFI::free($uv);
}

/** @param ffi_scope<example> $example_lib */
function init_array($example_lib, $size) {
  $lib = FFI::scope('pointers');

  $v = $lib->new("uint8_t[$size]", false);
  for ($i = 0; $i < $size; $i++) {
    ffi_array_set($v, $i, $i + 1);
  }

  $lib->set_unmanaged_data(FFI::addr($v));
  $uv = FFI::cast("uint8_t[$size]", $lib->get_unmanaged_data());
  for ($i = 0; $i < $size; $i++) {
    var_dump(ffi_array_get($uv, $i));
  }
}

/** @param ffi_scope<example> $example_lib */
function test_array($example_lib, $size) {
  $lib = FFI::scope('pointers');

  $v = $lib->new("uint8_t[$size]");
  $tmp = $lib->get_unmanaged_data();
  $uv = FFI::cast("uint8_t[$size]", $tmp);
  for ($i = 0; $i < $size; $i++) {
    var_dump(ffi_array_get($uv, $i));
  }

  FFI::free($uv);

  $v2 = $example_lib->new('struct Vector2[' . ($size * 3) . ']', false);
  for ($i = 0; $i < $size * 3; $i++) {
    $elem = $example_lib->new('struct Vector2');
    $elem->x = (float)($i * 10);
    $elem->y = (float)($i + 1);
    ffi_array_set($v2, $i, $elem);
  }
  $lib->set_unmanaged_data(FFI::addr($v2));
  $uv2 = $example_lib->cast('struct Vector2[' . ($size * 3) . ']', $lib->get_unmanaged_data());
  for ($i = 0; $i < $size * 3; $i++) {
    $elem = ffi_array_get($uv2, $i);
    echo "$elem->x . $elem->y\n";
  }
  FFI::free($v2);
}


/** @param ffi_scope<example> $example_lib */
function test_fixed_array($example_lib, $size) {
  $lib = FFI::scope('pointers');

  $v = $lib->new("uint8_t[6]");
  $tmp = $lib->get_unmanaged_data();
  $uv = FFI::cast("uint8_t[6]", $tmp);
  for ($i = 0; $i < count($uv); $i++) {
    var_dump(ffi_array_get($uv, $i));
  }

  FFI::free($uv);

  $v2 = $example_lib->new('struct Vector2[6]', false);
  for ($i = 0; $i < $size; $i++) {
    $elem = $example_lib->new('struct Vector2');
    $elem->x = (float)($i * 10);
    $elem->y = (float)($i + 1);
    ffi_array_set($v2, $i, $elem);
  }
  $lib->set_unmanaged_data(FFI::addr($v2));
  $uv2 = $example_lib->cast('struct Vector2[6]', $lib->get_unmanaged_data());
  for ($i = 0; $i < $size; $i++) {
    $elem = ffi_array_get($uv2, $i);
    echo "$elem->x . $elem->y\n";
  }
  FFI::free($v2);
}

function test_custom_alloc() {
  $lib = FFI::scope('pointers');

  var_dump($lib->alloc_callback_test(function ($size) {
    $mem = FFI::new("uint8_t[$size]", false);
    return FFI::cast('void*', FFI::addr($mem));
  }));

  var_dump($lib->alloc_callback_test2(function ($size) {
    $mem = FFI::new("uint8_t[$size]", false);
    return FFI::cast('void*', FFI::addr($mem));
  }));
}

$example_lib = FFI::cdef('
#define FFI_SCOPE "example"

struct Vector2 {
  double x;
  double y;
};
');

// Repeat several times to increase the chance of catching some unexpected crashes
for ($i = 0; $i < 10; $i++) {
  test_new_free();
  init_uint64();
  test_uint64();
  init_array($example_lib, 6);
  test_array($example_lib, 6);
  init_array($example_lib, 6);
  test_fixed_array($example_lib, 6);
}

test_custom_alloc();
