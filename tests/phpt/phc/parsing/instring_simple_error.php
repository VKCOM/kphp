@kphp_should_fail
/Cannot convert type \[class_instance<C\$Y>\] to string/
<?php

class X {
  public $y = 10;
  public $y2;
  public function __construct() {
    $this->y2 = new Y();
  }
}
class Y {
  public $z = 'z';
}

$x = new X();
echo "$x->y2->z";
