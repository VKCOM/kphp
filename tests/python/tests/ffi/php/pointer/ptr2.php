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

test();
test_ptr2_out_param();
