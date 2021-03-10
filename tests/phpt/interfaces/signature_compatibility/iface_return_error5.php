@kphp_should_fail
/Declaration of B::f\(\) must be compatible with A::f\(\)/
<?php

interface I {
  public function f();
}

class A implements I {
  public function f(): int {} // compatible with I::f
}

class B extends A {
  public function f(): string {} // compatible with I::f
}

