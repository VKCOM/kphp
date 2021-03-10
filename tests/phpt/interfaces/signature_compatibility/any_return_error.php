@kphp_should_fail
/Declaration of B::f\(\) must be compatible with A::f\(\)/
<?php

class Foo {}

abstract class A {
  /** @return Foo */
  public abstract function f();
}

class B extends A {
  /** @return any */
  public function f() { return 10; }
}
