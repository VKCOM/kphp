@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Iface::f\(\)/
/Iface parameter \$x type: A/
/Impl parameter \$x type: B/
<?php

class A {}
class B extends A {}

interface Iface {
  public function f(A $x);
}

class Impl implements Iface {
  public function f(B $x) {}
}

/** @var Iface[] $list */
$list = [new Impl()];
