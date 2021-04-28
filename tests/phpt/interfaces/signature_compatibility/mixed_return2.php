@ok
<?php

require_once 'kphp_tester_include.php';

class Foo {}

abstract class A {
  /** @return tuple(any, int) */
  public abstract function f();
}

class B extends A {
  /** @return tuple(Foo, int) */
  public function f() { return tuple(new Foo(), 10); }
}

class C extends A {
  /** @return tuple(string, int) */
  public function f() { return tuple("abc", 20); }
}

$b = new B();
$c = new C();
