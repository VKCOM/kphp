@ok
KPHP_REQUIRE_CLASS_TYPING=1
<?php

// check that @var accomplishes with default value

class A {
  /** @var A[]|false */
  static $cache = false;
}

A::$cache = [new A];
