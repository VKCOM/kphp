@kphp_should_fail
/Immutable class .*B.* has mutable base .*A.*/
<?php

class A {
  public $x = 1;
}

/**
 * @kphp-immutable-class
 */
class B extends A {
  public $y = 1;
}

$b = new B();
