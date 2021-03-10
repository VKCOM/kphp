@ok
<?php

class Foo {}
class Bar extends Foo {}

abstract class A {
  /** @param Bar $x */
  public abstract function f($x);
}

class B extends A {
  /** @param any $x */
  public function f($x) {}
}

class C extends A {
  /** @param Foo $x */
  public function f($x) {}
}

/** @var A[] $list */
$list = [new B(), new C()];
