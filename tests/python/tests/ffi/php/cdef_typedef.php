<?php

function test() {
  $cdef = FFI::cdef('
    typedef int8_t int8;
    typedef uint8_t uint8;

    typedef struct {
      int8 i8; // note: can not use int8 name here
      uint8 u8;
    } NumTypes;

    typedef struct Vector {
      double x;
      double y;
    } Vector;
  ');
  $x = $cdef->new('NumTypes');

  $x->i8 = 5423;
  var_dump($x->i8);
  $x->i8 = 34;
  var_dump($x->i8);

  $x->u8 = 5423;
  var_dump($x->u8);
  $x->u8 = 34;
  var_dump($x->u8);
  $x->u8 = -50;
  var_dump($x->u8);

  var_dump($x->i8 + $x->u8);
  var_dump($x->i8 - $x->u8);

  $v = $cdef->new('Vector');
  $v->x = 345.5;
  $v->y = 2.6;
  var_dump($v->x, $v->y);
}

test();
