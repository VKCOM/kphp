@ok php7_4
<?php

class Other {
  const TEST_CONST = 573;
  public static $x = 22;
}

class Example {
  public static function test_static($x) {
    $f = static fn() => $x * Other::TEST_CONST * Other::$x;
    var_dump($f());
  }

  public static function test_static_self($x) {
    $f = static fn() => $x * self::$x;
    var_dump($f());
  }

  private static $x = 1111;
}

function test_static() {
  $id = static fn($x) => $x;
  var_dump($id(32), $id("str"));
}

test_static();
Example::test_static(10);
Example::test_static_self(10);
