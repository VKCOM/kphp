@kphp_should_fail access
/Can't access/
<?php

class Base {
  private $field = [];
  private function fun1() { echo "Base::fun1\n";}
}

$x = new Base();
var_dump($x->field);
