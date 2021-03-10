@ok php7_4
<?php

class A {}
class B extends A {}

abstract class Base {
  public abstract static function f(B $x);
}

class Impl extends Base {
  public static function f(A $x) {}
}

Impl::f(new A());
/** @var Base[] $list */
$list = [new Impl()];
