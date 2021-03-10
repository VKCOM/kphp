@ok
<?php

class A {}

interface Iface {
  public function f(A $x);
  public function g(int $x);
}

class Impl implements Iface {
  public function f(A $x) {}
  public function g(int $x) {}
}

/** @var Iface[] $list */
$list = [new Impl()];
