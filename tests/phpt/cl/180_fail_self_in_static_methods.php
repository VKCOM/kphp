@kphp_should_fail
/Called instance method A::g\(\) using :: \(need to use ->\)/
<?php
class A {
  static function f() {
    self::g();
  }
  function g() {
  }
}

A::f();
