@ok php7_4
<?php

class A {}
class B extends A {}

interface Iface {
  public function g(): ?A;
}

class Impl implements Iface {
  public function g() : B { return new B(); }
}

/** @var Iface[] $list */
$list = [new Impl()];
