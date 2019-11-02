@kphp_should_fail
/currently disallowed/
<?php

class A {
  public $x = 1;
}

$a = new A;
echo null === $a;
