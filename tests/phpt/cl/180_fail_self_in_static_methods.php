@kphp_should_fail
/Non-static method A::g\(\) is called statically/
/Non-static method Aside::fff\(\) is called statically/
<?php
class A {
  static function f() {
    self::g();
  }
  function g() {
    Aside::fff();
  }
}

class Aside {
    function fff() {}
}

A::f();
