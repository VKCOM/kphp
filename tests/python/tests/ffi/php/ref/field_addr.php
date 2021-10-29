<?php

require_once __DIR__ . '/../include/common.php';

kphp_load_pointers_lib();

$cdef = FFI::cdef('
  struct Foo { int i; };
  struct Bar { struct Foo foo; };
  struct Baz { struct Foo *foo_ptr; };
');

$bar = $cdef->new('struct Bar');
$baz = $cdef->new('struct Baz');

$ptrlib = FFI::scope('pointers');

$foo_ref = $bar->foo;
$addr1 = $ptrlib->voidptr_addr_value(FFI::addr($bar->foo));
$addr2 = $ptrlib->voidptr_addr_value(FFI::addr($foo_ref));
$baz->foo_ptr = FFI::addr($bar->foo);
$addr3 = $ptrlib->voidptr_addr_value($baz->foo_ptr);
$baz->foo_ptr = FFI::addr($foo_ref);
$addr4 = $ptrlib->voidptr_addr_value($baz->foo_ptr);

var_dump(count(array_unique([$addr1, $addr2, $addr3, $addr4])));
