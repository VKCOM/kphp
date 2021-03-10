@kphp_should_fail
/Impl::f\(\) should not have a \$x param type hint, it would be incompatible with Base::f\(\)/
<?php

class A {}
class B extends A {}

abstract class Base {
  public abstract static function f($x);
}

class Impl extends Base {
  public static function f(int $x) {}
}

Impl::f(0);
/** @var Base[] $list */
$list = [new Impl()];
