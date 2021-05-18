@kphp_should_fail
/'use' should be used at the top of the file/
<?php

C::f();
use function Utils\rand;

class C {
  public static function f() {}
}
