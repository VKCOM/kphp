@ok
<?php

interface Iface {
  public function f(int $x);
}

class Impl implements Iface {
  public function f($x) {}
}

/** @var Iface[] $list */
$list = [new Impl()];
