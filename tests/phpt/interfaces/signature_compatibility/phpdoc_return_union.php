@ok
<?php

class A {}
class B {}

abstract class Base {
  /** @return int|string */
  public abstract function f();

  /** @return A|B */
  public abstract function g();
}

class Derived1 extends Base {
  /** @return int */
  public function f() { return 10; }

  /** @return A */
  public function g() { return new A(); }
}

class Derived2 extends Base {
  public function f(): string { return 'foo'; }

  public function g(): B { return new B(); }
}

/** @var Base[] $list */
$list = [new Derived1(), new Derived2()];
