@ok php7_4
<?php

class A {}
class B extends A {}

interface Iface {
  public function f(): A;
}

class Impl implements Iface {
  public function f(): B { return new B(); }
}

/** @var Iface[] $list */
$list = [new Impl()];
