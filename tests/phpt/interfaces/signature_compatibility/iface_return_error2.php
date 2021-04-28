@kphp_should_fail
/Impl::f\(\) should provide a return type hint compatible with Iface::f\(\)/
<?php

// If base method has a return type hint,
// derived method should have it as well.

interface Iface {
  public function f($x) : int;
}

class Impl implements Iface {
  public function f($x) {}
}

/** @var Iface[] $list */
$list = [new Impl()];
