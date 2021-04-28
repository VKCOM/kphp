@ok
<?php

class A {}
class B {}

abstract class Base {
  /** @param int $x */
  public abstract function f($x);

  /** @param A $x */
  public abstract function g($x);
}

class Derived1 extends Base {
  /** @param int|string $x */
  public function f($x) {}

  /** @param A|B $x */
  public function g($x) {}
}

class Derived2 extends Base {
  /** @param string|int $x */
  public function f($x) {}

  /** @param B|A $x */
  public function g($x) {}
}

/** @var Base[] $list */
$list = [new Derived1(), new Derived2()];
