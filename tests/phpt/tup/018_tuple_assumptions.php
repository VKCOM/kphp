@ok
<?php
require_once 'polyfills.php';

define('ZERO', 0);

class A {
  const ONE = 1;

  var $a = 0;
  public function __construct($a = 0) { $this->a = $a; }

  public function p() { echo 'A field = ' . $this->a . "\n"; }
}

class B {
  var $a = 0;
  public function __construct($a = 0) { $this->a = $a; }

  public function p() { echo 'B field = ' . $this->a . "\n"; }
}

class TupleContainer {
  /** @var tuple(A, B) */
  public $tup;

  function __construct() {
    $this->tup = tuple(new A(10), new B(20));
  }
}

/** @return tuple(A, B, int) */
function getTup() {
  return tuple(new A(1), new B(2), 6);
}

function demo_tuple_inplace() {
  [$a, $b, $int] = tuple(new A(1), new B(2), 6);
  echo $a->a, "\n";
  echo $b->a, "\n";
  echo $int, "\n";
}

function demo_tuple_get() {
  [$a, $b, $int] = getTup();
  echo $a->a, "\n";
  echo $b->a, "\n";
  echo $int, "\n";
}

function demo_tuple_get_v2() {
  $tup = getTup();
  echo $tup[0]->a, "\n";
  echo $tup[1]->a, "\n";
  echo $tup[2], "\n";
}

function demo_tuple_get_v3() {
  $tup = getTup();
  $tup[ZERO]->p();
  $tup[A::ONE]->p();
}

function demo_tuple_get_v4() {
  getTup()[0]->p();
  getTup()[1]->p();
}

function demo_tuple_lvalue_null() {
  [$a, ,$int] = getTup();
  echo $a->a, "\n";
  echo $int, "\n";
}

function demo_tuple_lvalue_null_v2() {
  [, $b,] = getTup();
  echo $b->a, "\n";
}

function demo_tuple_field() {
  $c = new TupleContainer();
  [$a, $b] = $c->tup;
  echo $a->a, "\n";
  echo $b->a, "\n";
}

/**
 * @param $tup tuple(A, B, int)
 */
function demo_tuple_param($tup) {
  [$a, $b, $int] = $tup;
  echo $a->a, "\n";
  echo $b->a, "\n";
  echo $int, "\n";
}

demo_tuple_inplace();
demo_tuple_get();
demo_tuple_get_v2();
demo_tuple_get_v3();
demo_tuple_get_v4();
demo_tuple_field();
demo_tuple_param(getTup());
demo_tuple_lvalue_null();
demo_tuple_lvalue_null_v2();


