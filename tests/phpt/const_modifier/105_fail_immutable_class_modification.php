@kphp_should_fail
/Modification of const/
<?php

/** @kphp-immutable-class */
class Y {
  public $v = 1;
}

/** @kphp-immutable-class */
class X {
  /** @var Y|false */
  public $y = false;
}

$x = new X;
$x->y->v = 2;
