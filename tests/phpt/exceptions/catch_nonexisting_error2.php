@kphp_should_fail
/Could not find catch class Blah\\Blah/
/Could not find catch class Throwable/
<?php

try {
  throw new Exception();
} catch (\Blah\Blah $e) {
}

class Foo {
  public static function f() {
    try {
      throw new Exception();
    } catch (Throwable $e) {
    }
  }
}

Foo::f();
