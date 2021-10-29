<?php

class Wrapper {
  /** @var ffi_cdata<example, struct Qux> */
  public $qux;

  /** @param ffi_cdata<example, struct Qux> $qux */
  public function __construct($qux) {
    $this->qux = $qux;
  }
};

class WrapperList {
  /** @var WrapperList */
  public $next;
  /** @var Wrapper */
  public $value;
};

function test() {
  $cdef = FFI::cdef('
    #define FFI_SCOPE "example"

    struct Foo {
      int64_t integer;
    };

    struct Bar {
      struct Foo foo;
    };

    struct Baz {
      struct Bar bar;
    };

    struct Qux {
      struct Baz baz;
    };

    typedef struct Qux Qux;
  ');

  $wrapper1 = new Wrapper($cdef->new('Qux'));
  $wrapper2 = new Wrapper($cdef->new('Qux'));

  $list = new WrapperList();
  $list->value = $wrapper1;
  $list->next = new WrapperList();
  $list->next->value = $wrapper2;

  $wrapper1->qux->baz->bar->foo->integer = 24;
  $wrapper2->qux->baz->bar->foo->integer = $wrapper1->qux->baz->bar->foo->integer + 1;

  $qux1 = $wrapper1->qux;
  $qux2 = $list->next->value->qux;

  var_dump($qux1->baz->bar->foo->integer);
  var_dump($wrapper1->qux->baz->bar->foo->integer);
  var_dump($list->value->qux->baz->bar->foo->integer);
  var_dump($qux2->baz->bar->foo->integer);
  var_dump($wrapper2->qux->baz->bar->foo->integer);
  var_dump($list->next->value->qux->baz->bar->foo->integer);

  $wrapper_array = [$wrapper1, $wrapper2];
  var_dump($wrapper_array[0]->qux->baz->bar->foo->integer);
  var_dump($wrapper_array[1]->qux->baz->bar->foo->integer);

  $baz1 = $wrapper_array[0]->qux->baz;
  $bar1 = $baz1->bar;
  var_dump($baz1->bar->foo->integer);
  var_dump($bar1->foo->integer);
}

test();
