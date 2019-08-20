@kphp_should_fail
/call abstract methods/
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
