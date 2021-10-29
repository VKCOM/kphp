<?php

function test1() {
  $cdef = FFI::cdef('
    typedef union {
      int32_t i32;
      int16_t i16;
    } Example;
  ');

  $u = $cdef->new('Example');
  $u->i32 = 14300130;
  var_dump($u->i16);
  var_dump($u->i32);
  $u->i16 = 999;
  var_dump($u->i16);
  var_dump($u->i32);
}

test1();
