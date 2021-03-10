@ok
<?php

// PHP allows to override ?T return type as T.
// But not the other way around.

class A {}

interface Iface {
  public function f($x) : ?int;
  public function g(): ?A;
}

class Impl implements Iface {
  public function f($x) : int { return 0; }
  public function g() : A { return new A(); }
}

/** @var Iface[] $list */
$list = [new Impl()];
