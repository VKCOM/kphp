@kphp_should_fail
/C::f\(\) should provide a return type hint compatible with B::f\(\)/
<?php

class A {
  public function f(): int {}
}

class B extends A {
  public function f(): int {}
}

class C extends B {
  public function f() {}
}
