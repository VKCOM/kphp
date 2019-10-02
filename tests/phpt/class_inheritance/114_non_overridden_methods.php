@kphp_should_fail
/You should override abstract method/
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

