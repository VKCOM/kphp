@kphp_should_fail
/Impl::f\(\) should not have a \$x param type hint, it would be incompatible with Iface::f\(\)/
<?php

interface Iface {
  public function f($x);
}

class Impl implements Iface {
  public function f(int $x) {}
}

/** @var Iface[] $list */
$list = [new Impl()];
