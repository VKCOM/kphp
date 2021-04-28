@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Iface::f\(\)/
<?php

interface Iface {
  public function f($x) : int;
}

class Impl implements Iface {
  public function f($x) : void {}
}

/** @var Iface[] $list */
$list = [new Impl()];
