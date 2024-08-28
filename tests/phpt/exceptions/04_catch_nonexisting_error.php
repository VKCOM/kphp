@kphp_should_fail k2_skip
/Can't find class: Foo\\Bar/
/Can't find class: Poof/
<?php

try {
  throw new Exception();
} catch (\Foo\Bar $e) {
  var_dump($e->getFile());
}

class Foo {
  public static function f() {
    try {
      throw new Exception();
    } catch (Poof $e) {
      var_dump($e->getLine());
    }
  }
}

Foo::f();
