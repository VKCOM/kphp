@ok
<?php

class Y {
  public $y_int = 123;
}

class X {
  /**
    * @var Y|null
    * @kphp-const
    */
  public $y = null;
}

$x = new X;
if($x->y) {
  $x->y->y_int = 10;
}
