@kphp_should_fail access
/Can't access/
<?php

class Base {
  static public function test() { Derived::foo(); }
}

class Derived extends Base {
  static private function foo() { echo "Derived::foo\n"; }
}

Base::test();
