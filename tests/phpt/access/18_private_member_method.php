@kphp_should_fail access
/Can't access/
<?php

class Base {
  private $field = [];
  private function fun1() { echo "Base::fun1\n";}
}

$x = new Base();
$x->fun1();
