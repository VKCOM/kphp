<?php

function test1() {
  // anon enum with default values
  $cdef = FFI::cdef('
    enum {
      A,
      B,
    };
    enum {
      C,
      D,
      E,
    };
  ');
  var_dump([$cdef->A, $cdef->B, $cdef->C, $cdef->D, $cdef->E]);
}

function test2() {
  // anon enum with full explicit initializers
  $cdef = FFI::cdef('
    enum {
      One = 1,
      Two = 2,
      Three = 3
    };
    enum {
      False = 0,
      True = 1
    };
    enum {
      Red = -1,
      Green = 0,
      Blue = +1
    };
  ');
  var_dump([
    $cdef->One, $cdef->Two, $cdef->Three,
    $cdef->False, $cdef->True,
    $cdef->Red, $cdef->Green, $cdef->Blue,
  ]);
}

function test3() {
  // anon enum with mixed initializers
  $cdef = FFI::cdef('
    enum {
      A,
      B = 3,
      C,
    };
    enum {
      X = 0,
      Y = 1,
      Z,
    };
    enum {
      Foo,
      Bar = 5,
      Baz = 6,
      Qux,
    };
  ');
  var_dump([
    $cdef->A, $cdef->B, $cdef->C,
    $cdef->X, $cdef->Y, $cdef->Z,
    $cdef->Foo, $cdef->Bar, $cdef->Bar, $cdef->Qux,
  ]);
}

function test4() {
  // unnamed enum + typedef
  $cdef = FFI::cdef('
    typedef enum {
      Red,
      Green,
      Blue,
    } Color;

    struct Foo {
      Color color;
    };
  ');

  var_dump([
    $cdef->Red, $cdef->Green, $cdef->Blue,
  ]);

  $foo = $cdef->new('struct Foo');
  $foo->color = 10;
  var_dump($foo->color);
  $foo->color = $cdef->Red;
  var_dump($foo->color);
  $foo->color += 1;
  var_dump($foo->color);
}

function test5() {
  // named enum
  $cdef = FFI::cdef('
      enum Color {
        Red,
        Green,
        Blue,
      };

      struct Foo {
        enum Color color;
      };
    ');

    var_dump([
      $cdef->Red, $cdef->Green, $cdef->Blue,
    ]);

    $foo = $cdef->new('struct Foo');
    $foo->color = 10;
    var_dump($foo->color);
    $foo->color = $cdef->Red;
    var_dump($foo->color);
    $foo->color += 1;
    var_dump($foo->color);
}

function test6() {
  // named enum + typedef
  $cdef = FFI::cdef('
    enum Color {
      Red,
      Green,
      Blue,
    };

    typedef enum Color Color;

    struct Foo {
      Color color;
    };
  ');

  var_dump([
    $cdef->Red, $cdef->Green, $cdef->Blue,
  ]);

  $foo = $cdef->new('struct Foo');
  $foo->color = 10;
  var_dump($foo->color);
  $foo->color = $cdef->Red;
  var_dump($foo->color);
  $foo->color += 1;
  var_dump($foo->color);
}

function test7() {
  // different numeric bases
  $cdef = FFI::cdef('
    enum {
      A = 0x20,
      B = 0xff,
    };
    enum {
      X = 010,
      Y = 025,
      Z,
    };
  ');

  var_dump([
    $cdef->A, $cdef->B,
    $cdef->X, $cdef->Y, $cdef->Z,
  ]);
}

test1();
test2();
test3();
test4();
test5();
test6();
test7();
