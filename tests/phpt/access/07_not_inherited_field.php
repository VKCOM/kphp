@kphp_should_fail access
/static field not found:/
<?php

class Base {
  static private $field = [];
  static private function fun1() { echo "Base::fun1\n";}
}

class Derived extends Base {

}

var_dump(Derived::$field);
