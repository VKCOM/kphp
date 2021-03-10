@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Iface::f\(\)/
<?php

// PHP allows to override ?T return type as T.

class A {}

interface Iface {
  public function f($x) : int;
  public function g(): A;
}

class Impl implements Iface {
  public function f($x) : ?int { return null; }
  public function g() : ?A { return null; }
}

/** @var Iface[] $list */
$list = [new Impl()];
