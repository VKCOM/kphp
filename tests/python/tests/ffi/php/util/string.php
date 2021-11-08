<?php

function test_with_len_uint8ptr() {
  $v = FFI::new('uint32_t');
  $v->cdata = 0x78787861;
  $ptr = FFI::cast('uint8_t*', FFI::addr($v));
  $s = FFI::string($ptr, 4);
  var_dump($s);
  var_dump($s === 'axxx');
}

function test_with_len_int16ptr() {
  $v = FFI::new('int16_t');
  $v->cdata = 0x6161;
  $s = FFI::string(FFI::addr($v), 2);
  var_dump($s);
  var_dump($s === 'aa');
}

function test_with_len_structptr() {
  $cdef = FFI::cdef('
    struct Foo {
      char c1;
      char c2;
      char c3;
    };
  ');
  $foo = $cdef->new('struct Foo');
  $foo->c1 = "f";
  $foo->c2 = "o";
  $foo->c3 = "o";
  $s = FFI::string(FFI::addr($foo), 3);
  var_dump($s);
  var_dump($s === 'foo');
}

function test_without_len() {
  $v = FFI::new('uint32_t');
  $v->cdata = 0x00617861;
  $ptr = FFI::cast('char*', FFI::addr($v));
  $s = FFI::string($ptr);
  var_dump($s);
  var_dump($s == 'axa');

  // TODO: add tests with `const char*` when #357 is resolved.
}

test_with_len_uint8ptr();
test_with_len_int16ptr();
test_with_len_structptr();
test_without_len();
