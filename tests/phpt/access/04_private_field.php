@kphp_should_fail access
/Can't access/
<?php

class Base {
  static private $field = [];
  static private function fun1() { echo "Base::fun1\n";}
}

var_dump(Base::$field);
