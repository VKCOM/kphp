@kphp_should_fail
/currently disallowed/
<?php

class A {
  public $x = 1;
}

interface I {
}

class B {
  /** @var I */
  public $a = null;
}

$b = new B;
$a = $b->a;
echo isset($a);

