@kphp_should_fail
/Declaration of Impl::f\(\) must be compatible with Iface::f\(\)/
<?php

// Documentation tells that this code *should* compile,
// but PHP7.2, PHP7.4 and PHP8.0 ignore this fact and fail to execute it;
// we reject it at the compile time to be compatible

interface Iface {
  public function f($x) : int|float;
}

class Impl implements Iface {
  public function f($x) : int { return 0; }
}

/** @var Iface[] $list */
$list = [new Impl()];
