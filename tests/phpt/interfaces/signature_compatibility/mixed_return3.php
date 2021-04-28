@ok
<?php

abstract class A {
  /** @return mixed|false */
  public abstract function f();
}

class B extends A {
  /** @return int|false */
  public function f() { return false; }
}

class C extends A {
  /** @return ?string|false */
  public function f() { return null; }
}

$b = new B();
$c = new C();
