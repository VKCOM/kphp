@kphp_should_fail
/pass mixed to argument \$x of ensure_string/
<?php

class Foo {}

function ensure_string(string $x) {}

class C1 {
  /** @var mixed $value */
  public static $value = null;
  public static function test() {
    if (is_string(self::$value)) {
      ensure_string(self::$value);
    }
  }
}

C1::test();
