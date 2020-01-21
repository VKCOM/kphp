@ok
<?php

class A {
  public $x = 1;
}

class B {
  /** @var A */
  public $a = null;
}

$b = new B;
echo is_null($b->a);

