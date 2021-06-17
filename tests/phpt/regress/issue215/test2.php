@ok
<?php

interface Iface {
  function f();
}

abstract class AbstractBase {
  public function f() {}
}

abstract class AbstractBase2 extends AbstractBase {}

abstract class AbstractBase3 extends AbstractBase2 {}

class Concrete extends AbstractBase3 implements Iface {}

$c = new Concrete();
