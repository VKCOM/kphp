@kphp_should_fail
/Converting to a string of a class Y that does not contain a __toString\(\) method/
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
