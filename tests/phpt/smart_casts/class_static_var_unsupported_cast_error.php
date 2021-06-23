@kphp_should_fail
/pass mixed to argument \$x of ensure_int/
<?php

class Foo {}

function ensure_int(int $x) {}

class C1 {
  /** @var mixed $value */
  public static $value = null;
  public static function test() {
    if (is_int(self::$value)) {
      ensure_int(self::$value);
    }
  }
}

C1::test();
