@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Iface::f\(\)/
<?php

class A {}

interface Iface {
  public function f($x) : int;
}

class Impl implements Iface {
  public function f($x) : float { return 0; }
}

/** @var Iface[] $list */
$list = [new Impl()];
