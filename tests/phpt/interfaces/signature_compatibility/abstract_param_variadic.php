@ok php7_4
<?php

// Variadic params follow the same rules.

class A {}
class B extends A {}

abstract class Base {
  public abstract function f(B ...$params);
  public abstract function g(int ...$params);
}

class Impl1 extends Base {
  public function f(A ...$params) {}
  public function g(?int ...$params) {}
}

class Impl2 extends Base {
  public function f(B ...$params) {}
  public function g(int ...$params) {}
}

/** @var Base[] $list */
$list = [new Impl1(), new Impl2()];
