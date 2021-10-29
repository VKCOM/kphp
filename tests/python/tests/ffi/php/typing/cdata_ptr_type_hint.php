<?php

class Wrapper {
  /** @var ffi_cdata<example, struct Foo*> */
  public $foo_ptr;

  /** @var ffi_cdata<example, struct Foo**> */
  public $foo_ptr2;
}

/** @param ffi_cdata<example, struct Foo*> $foo_ptr */
function print_field($foo_ptr) {
  var_dump($foo_ptr->x);
}

$cdef = FFI::cdef('
  #define FFI_SCOPE "example"
  struct Foo { int64_t x; };
');
$foo = $cdef->new('struct Foo');

print_field(FFI::addr($foo));
$foo->x = 1500;
print_field(FFI::addr($foo));

$wrapper = new Wrapper();
$wrapper->foo_ptr = FFI::addr($foo);
$wrapper->foo_ptr2 = FFI::addr($wrapper->foo_ptr);
