<?php

class Foo {
  /** @var \FFI\CData_int32 $x */
  public $int32;
}

/** @param \FFI\CData_int32 $x */
function print_cdata_int32($x) {
  var_dump($x->cdata);
}

function test_param() {
  $int = FFI::new('int32_t');
  print_cdata_int32($int);
  $int->cdata = 15;
  print_cdata_int32($int);
}

function test_class_field() {
  $int32 = FFI::new('int32_t');
  $foo = new Foo();
  $foo->int32 = $int32;
  $foo_int32 = $foo->int32;
  var_dump($foo_int32->cdata);
  $int32->cdata = 20;
  var_dump($foo_int32->cdata);
}

test_param();
test_class_field();
