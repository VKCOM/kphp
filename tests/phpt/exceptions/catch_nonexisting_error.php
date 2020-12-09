@kphp_should_fail
/Unknown function ->getFile\(\)/
/Unknown function ->getLine\(\)/
<?php

try {
  throw new Exception();
} catch (\Blah\Blah $e) {
  var_dump($e->getFile());
}

class Foo {
  public static function f() {
    try {
      throw new Exception();
    } catch (Throwable $e) {
      var_dump($e->getLine());
    }
  }
}

Foo::f();
