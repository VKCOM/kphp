@ok
<?php

abstract class A {
  /** @param mixed[]|mixed $x */
  public abstract function f($x);
}

class B extends A {
  /** @param mixed|mixed[] $x */
  public function f($x) {}
}

$b = new B();
