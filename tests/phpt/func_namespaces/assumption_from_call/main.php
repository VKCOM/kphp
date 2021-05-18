<?php

namespace D;

use A\B\C\Foo;

class Test {
  public static function f() {
    /** @var Foo */
    $foo = new_foo();
    var_dump($foo->value);
  }
}

Test::f();
