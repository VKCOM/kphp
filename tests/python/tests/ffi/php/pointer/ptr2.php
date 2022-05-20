<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_pointers_lib();

function test() {
    $cdef = FFI::cdef('
      struct Foo {
        int64_t val;
      };
      struct Bar {
        int64_t *ptr;
      };
      struct Baz {
        int64_t **ptr2;
      };
    ');

    $foo = $cdef->new('struct Foo');
    $bar = $cdef->new('struct Bar');
    $baz = $cdef->new('struct Baz');

    $foo->val = 54;

    $bar->ptr = FFI::addr($foo->val);
    $baz->ptr2 = FFI::addr($bar->ptr);

    $lib = FFI::scope('pointers');
    var_dump($lib->voidptr_addr_value($bar->ptr) !== $lib->voidptr_addr_value($baz->ptr2));
    var_dump($lib->read_int64($bar->ptr) === $foo->val);
    var_dump($lib->read_int64($lib->read_int64p($baz->ptr2)) === $foo->val);

    $ptr1 = $lib->read_int64p($baz->ptr2);
    var_dump($lib->voidptr_addr_value($ptr1) === $lib->voidptr_addr_value($bar->ptr));
    $ptr2 = $bar->ptr;
    var_dump($lib->voidptr_addr_value($ptr1) === $lib->voidptr_addr_value($ptr2));
}

function test_ptr2_out_param() {
  $lib = FFI::scope('pointers');

  $out_ptr = FFI::new('const char*');
  $lib->cstr_out_param(FFI::addr($out_ptr));
  var_dump(FFI::string($out_ptr));
}

function test2() {
    $lib = FFI::scope('pointers');
    $u64 = $lib->new('uint64_t');
    $lib->uint64_memset(FFI::addr($u64), 1);
    var_dump($u64->cdata == 0x101010101010101);
    var_dump($u64->cdata);
    $lib->uint64_memset(FFI::addr($u64), 2);
    var_dump($u64->cdata == 0x202020202020202);
    var_dump($u64->cdata);
}

function test3() {
  $lib = FFI::scope('pointers');
  $i64 = $lib->new('int64_t');
  $i64->cdata = 240;
  $voidptr = $lib->new('VoidPtr');
  $lib->set_void_ptr(FFI::addr($voidptr), FFI::addr($i64));
  var_dump($lib->read_int64_from_void($voidptr));
}

test();
test2();
test_ptr2_out_param();
test3();
