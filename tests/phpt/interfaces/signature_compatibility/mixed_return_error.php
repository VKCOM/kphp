@kphp_should_fail
/Declaration of B::f\(\) must be compatible with A::f\(\)/
<?php

require_once 'kphp_tester_include.php';

abstract class A {
  /** @return mixed */
  public abstract function f();
}

class B extends A {
  /** @return tuple(int) */
  public function f() { return tuple(1); }
}

$b = new B();
