@kphp_should_fail
/Unknown method/
<?php
abstract class A {
  abstract public function foo();
}

class B extends A {
  public function foo() {
    parent::foo();
  }
}

$a = new B;
$a->foo();
