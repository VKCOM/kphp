@ok
<?php

require_once 'polyfills.php';

class A {
  /** @var int */
  public $x;
  public function __construct($x) { $this->x = $x; }
}

interface B {
  public function foo();
}

class C implements B {
  public function foo() { echo "I'm C\n";}
}

class D implements B {
  /** @var int */
  public $x;
  public function __construct($x) { $this->x = $x; }
  public function foo() { echo "I'm D, x = {$this->x}\n";}
}

function return_A() { return new A(4);}
function return_C() { return new C();}
function return_D() { return new D(5);}

function test() {
  /** @var A */
  $a = wait_result(fork(return_A()));
  var_dump($a->x);

  /** @var B */
  $c = wait_result(fork(return_C()));
  $c->foo();

  $c = wait_result(fork(return_D()));
  $c->foo();

  $r = [fork(return_C()), fork(return_D())];
  $c = wait_result($r[0]);
  $c->foo();
  $c = wait_result($r[1]);
  $c->foo();
}

test();
