@kphp_should_warn
/is always false/
<?php

class A {
  public $x = 1;
}

$a = new A;
echo $a === false;
