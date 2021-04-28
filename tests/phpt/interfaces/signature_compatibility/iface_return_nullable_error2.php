@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Iface::f\(\)/
<?php

class A {}
class B {}

interface Iface {
  public function f($x) : ?A;
}

class Impl implements Iface {
  public function f($x) : B {}
}

/** @var Iface[] $list */
$list = [new Impl()];
