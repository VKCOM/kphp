@ok
KPHP_REQUIRE_FUNCTIONS_TYPING=1
<?php

class A {
  /**
   * @param int $x
   * @param string $y
   */
  public static function f($x, $y) {
  }
}

class B extends A {}

class C extends B {}

A::f(1, 'foo');
B::f(1, 'foo');
C::f(1, 'foo');
