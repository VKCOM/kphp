@kphp_should_fail
/C::f\(\) should not have a \$y param type hint, it would be incompatible with B::f\(\)/
<?php

class A {
  public function f(int $x, $y) {}
}

class B extends A {
  public function f(int $x, $y) {}
}

class C extends B {
  public function f(int $x, int $y) {}
}
