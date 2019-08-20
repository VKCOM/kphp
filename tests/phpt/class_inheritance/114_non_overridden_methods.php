@kphp_should_fail
/must be declared abstract/
<?php

abstract class A {
  abstract public function foo();
}

class B extends A {
}

class C extends B {
  final public function foo() {
    echo "hello from c\n";
  }
}

