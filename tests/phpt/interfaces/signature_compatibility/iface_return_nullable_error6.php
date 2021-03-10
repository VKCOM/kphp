@kphp_should_fail
KPHP_REQUIRE_FUNCTIONS_TYPING=1
/Declaration of Foo::stringOrNull\(\) must be compatible with Iface::stringOrNull\(\)/
<?php

// In PHP, a standalone `null` type can't be used.

interface Iface {
  /** @return ?string */
  public static function stringOrNull();
}

class Foo implements Iface {
  /** @return null */
  public static function stringOrNull() { return null; }
}

$_ = new Foo();
var_dump(Foo::stringOrNull());
