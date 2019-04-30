@kphp_should_fail
/Modification of const/
<?php

class Y {
  public $y_int = 123;
}

class X {
  /**
    * @var Y|false
    * @kphp-const
    */
  public $y = false;
}

$x = new X;
if(!$x->y) {
  $x->y = new Y;
}
