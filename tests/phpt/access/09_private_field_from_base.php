@kphp_should_fail access
/Can't access/
<?php

class Base {
  static public function test() { var_dump(Derived::$field); }
}

class Derived extends Base {
  static private $field = [];
}

Base::test();
