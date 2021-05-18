<?php

namespace A\B\C;

class Foo {
  const X = 34;

  public static function f() {
    var_dump(self::X);
    var_dump(Foo::X);
    var_dump(\A\B\C\Foo::X);
  }
}
