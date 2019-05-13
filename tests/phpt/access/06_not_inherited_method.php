@kphp_should_fail access
/Can't access/
<?php

class Base {
  static private $field = [];
  static private function fun1() { echo "Base::fun1\n";}
}

class Derived extends Base {

}

Derived::fun1();
