@ok
<?php
require_once 'polyfills.php';

define('B_NAME', 'b');

class A {
  const A_NAME = 'a';

  var $a = 0;
  public function __construct($a = 0) { $this->a = $a; }

  public function p() { echo 'A field = ' . $this->a . "\n"; }
}

class B {
  var $a = 0;
  public function __construct($a = 0) { $this->a = $a; }

  public function p() { echo 'B field = ' . $this->a . "\n"; }
}

class ShapeContainer {
  /** @var shape(a:A, b:B) */
  public $sh;

  function __construct() {
    $this->sh = shape(['a' => new A(10), 'b' => new B(20)]);
  }
}

/** @return shape(a:A, b:B, i:int) */
function getShape() {
  return shape(['a' => new A(1), 'i' => 6, 'b' => new B(2)]);
}

function demo_shape_inplace() {
  $sh = shape(['a' => new A(1), 'b' => new B(2), 'i' => 6]);
  $a = $sh['a'];
  echo $a->a, "\n";
  echo $sh['b']->a, "\n";
  echo $sh['i'], "\n";
}

function demo_shape_get() {
  $sh = getShape();
  $a = $sh['a'];
  echo $a->a, "\n";
  echo $sh['b']->a, "\n";
  echo $sh['i'], "\n";
}

function demo_shape_get_v2() {
  $sh = getShape();
  $sh[B_NAME]->p();
  $sh[A::A_NAME]->p();
}

function demo_shape_get_v3() {
  getShape()['a']->p();
  getShape()['b']->p();
}

function demo_shape_field() {
  $c = new ShapeContainer();
  $a = $c->sh['a'];
  echo $a->a, "\n";
  echo $c->sh['b']->a, "\n";
}

/**
 * @param $sh shape(a:A, b:B, i:int)
 */
function demo_shape_param($sh) {
  $a = $sh['a'];
  echo $a->a, "\n";
  echo $sh['a']->a, "\n";
  echo $sh['b']->a, "\n";
  echo $sh['i'], "\n";
}

demo_shape_inplace();
demo_shape_get();
demo_shape_get_v2();
demo_shape_get_v3();
demo_shape_field();
demo_shape_param(getShape());

