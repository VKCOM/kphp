@kphp_should_fail
/but this class is abstract/
<?php

abstract class A {
  abstract public function foo();
}

$a = new A;
