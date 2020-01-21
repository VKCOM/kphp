@kphp_should_fail
/mix class A with false/
<?php

class A {
  public $x = 1;
}

$a = new A;
if (0) {
  $a = false;
}

echo !!$a;
