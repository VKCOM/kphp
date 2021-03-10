@kphp_should_fail
/B::f\(\) should not have a \$x param type hint, it would be incompatible with A::f\(\)/
<?php

class A {
  /** @param float $x */
  public function f($x) {}
}

class B extends A {
  public function f(int $x) {}
}

$b = new B();
