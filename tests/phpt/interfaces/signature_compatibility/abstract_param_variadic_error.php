@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Base::f\(\)/
<?php

class A {}

abstract class Base {
  public abstract function f(A ...$params);
}

class Impl extends Base {
  public function f(A $params) {}
}

/** @var Base[] $list */
$list = [new Impl()];
