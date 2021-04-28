@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Base::f\(\)/
<?php

class A {}
class B {}

abstract class Base {
  public abstract function f(): A;
}

class Impl extends Base {
  public function f(): B { return new B(); }
}

/** @var Base[] $list */
$list = [new Impl()];
