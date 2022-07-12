<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_pointers_lib();

function test_null_cstr() {
  $lib = FFI::scope('pointers');
  var_dump($lib->nullptr_cstr());
  var_dump($lib->nullptr_cstr() === null);
  var_dump($lib->empty_cstr());
  var_dump($lib->empty_cstr() === "");
}

function test_array_read() {
  $arr = FFI::new('const char*[2]');
  var_dump(ffi_array_get($arr, 0));

  $s = FFI::new('char[10]');
  ffi_memcpy_string($s, 'hello', 5);

  ffi_array_set($arr, 1, FFI::cast('const char*', $s));
  var_dump(ffi_array_get($arr, 1));

  $empty_s = FFI::new('char[1]'); // null byte
  ffi_array_set($arr, 1, FFI::cast('const char*', $empty_s));
  var_dump(ffi_array_get($arr, 1));
}

function test_field_read() {
  $cdef = FFI::cdef('
    struct WithCstr {
      const char *s;
    };
  ');
  $with_cstr = $cdef->new('struct WithCstr');
  var_dump($with_cstr->s); // NULL

  $s = FFI::new('char[10]');
  ffi_memcpy_string($s, 'hello', 5);
  $with_cstr->s = FFI::cast('const char*', $s);
  var_dump($with_cstr->s);

  $empty_s = FFI::new('char[1]'); // null byte
  $with_cstr->s = FFI::cast('const char*', $empty_s);
  var_dump($with_cstr->s);
}

test_null_cstr();
test_array_read();
test_field_read();
