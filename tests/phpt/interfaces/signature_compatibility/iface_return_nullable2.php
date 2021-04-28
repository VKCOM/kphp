@ok
<?php

class A {}

interface Iface {
  public function f($x) : ?int;
  public function g(): ?A;
}

class Impl implements Iface {
  public function f($x) : ?int { return null; }
  public function g() : ?A { return null; }
}

/** @var Iface[] $list */
$list = [new Impl()];
