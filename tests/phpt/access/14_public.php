@ok access
<?php

class Base {
  public $field = [];
  public function fun1() { echo "Base::fun1\n";}
}

$x = new Base();
$x->field[] = 1;
var_dump($x->field);
$x->fun1();
