<?php

function test3() {
  $cdef = FFI::cdef('
    union Value {
      int32_t int_val;
      bool bool_val;
    };
    struct Foo {
      union Value val;
    };
  ');

  $foo = $cdef->new('struct Foo');
  $foo->val->int_val = 1;
  var_dump($foo->val->int_val);
  var_dump($foo->val->bool_val);
  $foo->val->bool_val = false;
  var_dump($foo->val->int_val);
  var_dump($foo->val->bool_val);
  $foo->val->int_val = 124;
  var_dump($foo->val->int_val);
}

test3();
