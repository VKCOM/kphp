@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

class A {
  /** @var int */
  public $x;
  /**
   * @param int $x
   */
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
  /**
   * @param int $x
   */
  public function __construct($x) { $this->x = $x; }
  public function foo() { echo "I'm D, x = {$this->x}\n";}
}

/**
 * @return A
 */
function return_A() { return new A(4);}
/**
 * @return C
 */
function return_C() { return new C();}
/**
 * @return D
 */
function return_D() { return new D(5);}

function test() {
  /** @var A */
  $a = wait(fork(return_A()));
  var_dump($a->x);

  /** @var B */
  $c = wait(fork(return_C()));
  $c->foo();

  $c = wait(fork(return_D()));
  $c->foo();

  $r = [fork(return_C()), fork(return_D())];
  $c = wait($r[0]);
  $c->foo();
  $c = wait($r[1]);
  $c->foo();
}

test();
