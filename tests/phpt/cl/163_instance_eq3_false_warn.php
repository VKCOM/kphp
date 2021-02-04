@kphp_should_warn
/Types A and false can.t be identical/
<?php

class A {
  public $x = 1;
}

$a = new A;
echo $a === false;
