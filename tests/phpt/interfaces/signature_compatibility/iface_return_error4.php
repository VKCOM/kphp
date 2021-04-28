@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Iface::f\(\)/
<?php

class A {}
class B extends A {}

interface Iface {
  public function f($x) : B;
}

class Impl implements Iface {
  public function f($x) : A {}
}

/** @var Iface[] $list */
$list = [new Impl()];
