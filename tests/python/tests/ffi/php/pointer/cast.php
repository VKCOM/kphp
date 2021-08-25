<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_pointers_lib();

function f(string $s) {}

function test_cast_to_self() {
  $lib = FFI::scope('pointers');

  $int64 = FFI::new('int64_t');
  $int64_ptr = FFI::addr($int64);
  $int64_ptr2 = FFI::cast('int64_t*', $int64_ptr);
  $int64_ptr3 = FFI::cast('int64_t*', $int64_ptr2);
  var_dump($int64->cdata, $lib->read_int64($int64_ptr), $lib->read_int64($int64_ptr2), $lib->read_int64($int64_ptr3));
  $int64->cdata = 14;
  var_dump($int64->cdata, $lib->read_int64($int64_ptr), $lib->read_int64($int64_ptr2), $lib->read_int64($int64_ptr3));
  $lib->write_int64($int64_ptr3, 25);
  var_dump($int64->cdata, $lib->read_int64($int64_ptr), $lib->read_int64($int64_ptr2), $lib->read_int64($int64_ptr3));
}

function test_cast_void_pointer() {
  // Everything is compatible with void*, but void*
  // is only compatible with itself.
  // Sometimes we want to cast void* to a specific T*.
  // This test checks that it actually works.

  $lib = FFI::scope('pointers');

  $int32 = FFI::new('int32_t');
  $int32->cdata = 0x01020304;

  $as_bytes = FFI::cast('uint8_t*', FFI::addr($int32));
  $as_void = $lib->ptr_to_void($as_bytes);
  $back_as_bytes = FFI::cast('uint8_t*', $as_void);
  var_dump($lib->bytes_array_get($back_as_bytes, 0));
  var_dump($lib->bytes_array_get($back_as_bytes, 2));

  $int32->cdata = 0x00010002;
  var_dump($lib->bytes_array_get($as_bytes, 0));
  var_dump($lib->bytes_array_get($as_bytes, 1));
  var_dump($lib->bytes_array_get($as_bytes, 2));
  var_dump($lib->bytes_array_get($back_as_bytes, 0));
  var_dump($lib->bytes_array_get($back_as_bytes, 1));
  var_dump($lib->bytes_array_get($back_as_bytes, 2));

  $as_void2 = FFI::cast('void*', $as_bytes);
  $as_void3 = FFI::cast('void*', $as_void);
  var_dump($lib->voidptr_addr_value($as_void2) === $lib->voidptr_addr_value($as_void3));
  var_dump($lib->voidptr_addr_value($as_void2) === $lib->voidptr_addr_value($as_void));
  var_dump($lib->voidptr_addr_value($as_void2) === $lib->voidptr_addr_value($as_bytes));

  $as_void2 = FFI::cast('void*', $as_bytes);
  $back_as_bytes2 = FFI::cast('uint8_t*', $as_void2);
  var_dump($lib->bytes_array_get($back_as_bytes2, 0));
  var_dump($lib->bytes_array_get($back_as_bytes2, 1));
  var_dump($lib->bytes_array_get($back_as_bytes2, 2));
  var_dump($lib->bytes_array_get($back_as_bytes2, 3));
}

function test_cast_scalar_pointer() {
  $lib = FFI::scope('pointers');

  $int32 = FFI::new('int32_t');
  $int32->cdata = 0x01020304;
  $as_bytes = FFI::cast('uint8_t*', FFI::addr($int32));
  $arr = [
    $lib->bytes_array_get(FFI::cast('uint8_t*', FFI::addr($int32)), 0),
    $lib->bytes_array_get($as_bytes, 1),
    $lib->bytes_array_get($as_bytes, 2),
    $lib->bytes_array_get($as_bytes, 3),
  ];
  var_dump($arr);

  // Test that as_bytes reflect the changes in the int32 variable.
  $int32->cdata = 0x04030201;
  $arr2 = [
    $lib->bytes_array_get(FFI::cast('uint8_t*', FFI::addr($int32)), 0),
    $lib->bytes_array_get($as_bytes, 1),
    $lib->bytes_array_get($as_bytes, 2),
    $lib->bytes_array_get($as_bytes, 3),
  ];
  var_dump($arr, $arr2);

  // Now change the int32 value via the int8* pointer.
  var_dump($int32->cdata);
  $lib->bytes_array_set($as_bytes, 0, 10);
  var_dump($int32->cdata);
  $lib->bytes_array_set($as_bytes, 3, 10);
  var_dump($int32->cdata);
}

function test_cast_scalar_non_pointer() {
  // Casting the scalar types does the truncation.
  $int32 = FFI::new('int32_t');
  $int32->cdata = 0xffff;
  $int8 = FFI::cast('int8_t', $int32);
  var_dump($int32->cdata, $int8->cdata);
  $int32->cdata = 15;
  var_dump($int32->cdata, $int8->cdata);
}

function test_cast_struct_pointer() {
  // Casting T1* to T2*.

  $cdef1 = FFI::cdef('
    struct SingleInt { int x; };
    struct SingleIntWrapper { struct SingleInt *single_int; };
  ');
  $cdef2 = FFI::cdef('
    struct Foo { int value; };
    struct FooWrapper { struct Foo *foo; };
  ');

  $single_int = $cdef1->new('struct SingleInt');
  $single_int->x = 10;
  $as_foo = $cdef2->cast('struct Foo*', FFI::addr($single_int));
  var_dump([$single_int->x, $as_foo->value]);
  $as_foo->value = 20;
  var_dump([$single_int->x, $as_foo->value]);
  $single_int->x = 30;
  var_dump([$single_int->x, $as_foo->value]);

  $foo_wrapper = $cdef2->new('struct FooWrapper');
  $foo_wrapper->foo = $cdef2->cast('struct Foo*', $as_foo);
  var_dump($foo_wrapper->foo->value);
  $foo_wrapper->foo->value = 805;
  var_dump([$foo_wrapper->foo->value, $single_int->x, $as_foo->value]);

  $foo_wrapper->foo = $cdef2->cast('struct Foo*', FFI::addr($single_int));
  var_dump($foo_wrapper->foo->value);
  $foo_wrapper->foo->value = 95;
  var_dump([$foo_wrapper->foo->value, $single_int->x, $as_foo->value]);

  $foo = $cdef2->new('struct Foo');
  $foo->value = 111;
  $single_int_wrapper = $cdef1->new('struct SingleIntWrapper');
  $single_int_wrapper->single_int = $cdef1->cast('struct SingleInt*', FFI::addr($foo));
  var_dump([$foo->value, $single_int_wrapper->single_int->x]);
  $as_foo2 = $cdef2->cast('struct Foo*', $single_int_wrapper->single_int);
  var_dump([$as_foo2->value, $foo->value, $single_int_wrapper->single_int->x]);
}

function test_cast_struct_non_pointer() {
  // Casting T1 to T2; note that T1 and T2 are not pointer types.

  $cdef1 = FFI::cdef('
    struct SingleInt { int x; };
    struct SingleIntWrapper { struct SingleInt single_int; };
  ');
  $cdef2 = FFI::cdef('
    struct Foo { int value; };
    struct FooWrapper { struct Foo foo; };
  ');

  $single_int = $cdef1->new('struct SingleInt');
  $single_int->x = 10;
  $as_foo = $cdef2->cast('struct Foo', $single_int);
  var_dump([$single_int->x, $as_foo->value]);
  $as_foo->value = 20;
  var_dump([$single_int->x, $as_foo->value]);
  $single_int->x = 30;
  var_dump([$single_int->x, $as_foo->value]);

  $foo_wrapper = $cdef2->new('struct FooWrapper');
  $foo_wrapper->foo = clone $cdef2->cast('struct Foo', $as_foo);
  var_dump($foo_wrapper->foo->value);
  $foo_wrapper->foo->value = 805;
  var_dump([$foo_wrapper->foo->value, $single_int->x, $as_foo->value]);

  $foo_wrapper->foo = clone $cdef2->cast('struct Foo', $single_int);
  var_dump($foo_wrapper->foo->value);
  $foo_wrapper->foo->value = 95;
  var_dump([$foo_wrapper->foo->value, $single_int->x, $as_foo->value]);

  $foo = $cdef2->new('struct Foo');
  $foo->value = 111;
  $single_int_wrapper = $cdef1->new('struct SingleIntWrapper');
  $single_int_wrapper->single_int = clone $cdef1->cast('struct SingleInt', $foo);
  var_dump([$foo->value, $single_int_wrapper->single_int->x]);
  $as_foo2 = $cdef2->cast('struct Foo', $single_int_wrapper->single_int);
  var_dump([$as_foo2->value, $foo->value, $single_int_wrapper->single_int->x]);
  $as_foo2->value = 222;
  var_dump([$as_foo2->value, $foo->value, $single_int_wrapper->single_int->x]);
}

test_cast_to_self();
test_cast_void_pointer();
test_cast_scalar_pointer();
test_cast_scalar_non_pointer();
test_cast_struct_pointer();
test_cast_struct_non_pointer();
