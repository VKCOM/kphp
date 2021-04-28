@kphp_should_fail
/Declaration of Foo::stringOrNull\(\) must be compatible with Iface::stringOrNull\(\)/
<?php

// In PHP, a standalone `null` type can't be used.

interface Iface {
  /** @return string|null */
  public static function stringOrNull();
}

class Foo implements Iface {
  public static function stringOrNull(): null { return null; }
}

$_ = new Foo();
var_dump(Foo::stringOrNull());
