@kphp_should_fail
/Declaration of B::f\(\) must be compatible with A::f\(\)/
<?php

// Works in PHP, but fails in KPHP

class A {
  /** @return float */
  public function f() {
    return 0.5;
  }
}

class B extends A {
  public function f(): int {
    return 0;
  }
}

$b = new B();
