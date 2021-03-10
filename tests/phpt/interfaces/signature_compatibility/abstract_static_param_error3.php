@kphp_should_fail
/Impl::f\(\) should not have a \$p1 param type hint, it would be incompatible with Base::f\(\)/
<?php

class A {}
class B extends A {}

// Params $p1 and $p2 are swapped on purpose.

abstract class Base {
  public abstract static function f($p1, $p2);
}

class Impl extends Base {
  public static function f($p2, int $p1) {}
}

Impl::f(0, 0);
/** @var Base[] $list */
$list = [new Impl()];
