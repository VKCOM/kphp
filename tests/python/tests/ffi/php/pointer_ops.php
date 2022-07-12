<?php

require_once __DIR__ . '/include/common.php';

kphp_load_pointers_lib();

function test_assert(int $line, bool $v) {
  if (!$v) {
    echo "assertion failed as line $line\n";
  }
}

/**
 * @param ffi_cdata<C, uint8_t*> $ptr
 */
function uint8_ptr_sum($ptr, $num) {
  $sum = 0;
  for ($i = 0; $i < $num; $i++) {
    $sum += ffi_array_get($ptr, $i);
  }
  return $sum;
}

/**
 * @param ffi_cdata<C, const uint8_t*> $ptr
 */
function const_uint8_ptr_sum($ptr, $num) {
  $sum = 0;
  for ($i = 0; $i < $num; $i++) {
    $sum += ffi_array_get($ptr, $i);
  }
  return $sum;
}

function test() {
  $mem = FFI::new('uint8_t[16]');

  $uint64ptr = FFI::cast('uint64_t*', FFI::addr($mem));
  test_assert(__LINE__, ffi_array_get($uint64ptr, 0) === 0);
  ffi_array_set($uint64ptr, 0, 35);
  test_assert(__LINE__, ffi_array_get($uint64ptr, 0) === 35);
  for ($i = 0; $i < 8; $i++) {
    var_dump(ffi_array_get($mem, $i));
  }
  ffi_array_set($mem, 1, 12);
  var_dump(ffi_array_get($mem, 1));
  var_dump(ffi_array_get($uint64ptr, 0));
  var_dump(ffi_array_get($uint64ptr, 1));

  $uint8ptr = FFI::cast('uint8_t*', FFI::addr($mem));
  var_dump(uint8_ptr_sum($uint8ptr, 2));
  var_dump(const_uint8_ptr_sum($uint8ptr, 2));

  ffi_array_set($uint64ptr, 1, 43234);
  var_dump(ffi_array_get($uint64ptr, 0));
  var_dump(ffi_array_get($uint64ptr, 1));
  for ($i = 0; $i < count($mem); $i++) {
    var_dump(ffi_array_get($mem, $i));
    var_dump(ffi_array_get($uint8ptr, $i));
  }
}

test();
