@ok
<?php

class Foo {}

abstract class A {
  /** @return any */
  public abstract function f();
}

class B extends A {
  /** @return Foo */
  public function f() { return new Foo(); }
}

class C extends A {
  /** @return int */
  public function f() { return 50; }
}

class D extends A {
  /** @return int[] */
  public function f() { return [1]; }
}

/** @var A[] $list */
$list = [new B(), new C(), new D()];
