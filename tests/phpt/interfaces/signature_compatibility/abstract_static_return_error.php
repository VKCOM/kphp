@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Base::f\(\)/
<?php

class A {}
class B {}

abstract class Base {
  public abstract static function f(): A;
}

class Impl extends Base {
  public static function f(): B { return new B(); }
}

$_ = Impl::f();

/** @var Base[] $list */
$list = [new Impl()];
