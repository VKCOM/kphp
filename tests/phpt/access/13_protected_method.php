@kphp_should_fail access
/Can't access/
<?php

class Base {
  static protected $field = [];
  static protected function fun1() { echo "Base::fun1\n";}
}

Base::fun1();
