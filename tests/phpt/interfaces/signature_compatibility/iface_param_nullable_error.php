@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Iface::f\(\)/
<?php

class A {}

interface Iface {
  public function f(?int $x);
}

class Impl implements Iface {
  public function f(int $x) {}
}

/** @var Iface[] $list */
$list = [new Impl()];
