@kphp_should_fail
/prohibited/
<?php

class A {
  public $x = 1;
}

$a = new A;
echo $a === false;
