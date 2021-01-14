@kphp_should_fail
/Can't find class: Foo\\Bar/
/Can't find class: Poof/
<?php

try {
  throw new Exception();
} catch (\Foo\Bar $e) {
}

class Foo {
  public static function f() {
    try {
      throw new Exception();
    } catch (Poof $e) {
    }
  }
}

Foo::f();
