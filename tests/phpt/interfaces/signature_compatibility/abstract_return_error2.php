@kphp_should_fail
/Declaration of C::f\(\) must be compatible with B::f\(\)/
<?php

abstract class A {
  abstract function f();
}

class B extends A {
  function f(): int {}
}

class C extends B {
  function f(): string {}
}
