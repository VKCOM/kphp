<?php

interface FooInterface {
  public function foo();
}

trait FooTrait {
  public function foo() {
    return 1;
  }
}

abstract class AbstractFoo implements FooInterface {
    use FooTrait;
}

class Foo extends AbstractFoo {
}

var_dump((new Foo())->foo());
