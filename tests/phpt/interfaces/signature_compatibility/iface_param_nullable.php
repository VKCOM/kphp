@ok
<?php

class A {}

interface Iface {
  public function f(?int $x);
  public function g(?A $x);
}

class Impl implements Iface {
  public function f(?int $x) {}
  public function g(?A $x) {}
}

/** @var Iface[] $list */
$list = [new Impl()];
