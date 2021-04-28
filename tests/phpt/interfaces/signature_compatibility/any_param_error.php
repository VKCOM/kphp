@kphp_should_fail
/Declaration of B::f\(\) must be compatible with A::f\(\)/
<?php

class Foo {}

abstract class A {
  /** @return any */
  public abstract function f($x);
}

class B extends A {
  /** @param int $x */
  public function f($x) {}
}

$_ = new B();
