@ok
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


trait T {
    /** @var self */
    public $s = null;
}

abstract class A1 {
    use T;

    function f() {}
}

class A extends A1 {
}

$a = new A;
$a->s = new A;
$a->s->f();
