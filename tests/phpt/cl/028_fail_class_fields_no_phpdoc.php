@kphp_should_fail
/Incorrect type of class field/
<?php

class A {
  public $a = 0;
}

$a = new A;
$a->a = 'str';
