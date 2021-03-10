@ok
<?php

class A {}

interface Iface {
  public function f() : array;
  public function g() : A;
}

class Impl implements Iface {
  public function f() : array { return 0; }
  public function g() : A { return new A(); }
}

/** @var Iface[] $list */
$list = [new Impl()];
