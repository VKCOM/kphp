@ok
<?php

interface Iface {
  function f();
}

abstract class AbstractBase {
  public function f() {}
}

abstract class AbstractBase2 extends AbstractBase {}

class Concrete extends AbstractBase2 implements Iface {}

$c = new Concrete();
