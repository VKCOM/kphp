@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Base::f\(\)/
/Base parameter \$x type: B/
/Impl parameter \$x type: int/
<?php

class A {}
class B extends A {}

abstract class Base {
  public abstract static function f(B $x);
}

class Impl extends Base {
  public static function f(int $x) {}
}

Impl::f(0);
/** @var Base[] $list */
$list = [new Impl()];
