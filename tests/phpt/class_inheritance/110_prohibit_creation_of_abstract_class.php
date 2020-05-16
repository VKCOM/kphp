@kphp_should_fail
/Cannot instantiate abstract class A/
<?php

abstract class A {
  abstract public function foo();
}

$a = new A;
