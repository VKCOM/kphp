@kphp_should_fail
/Modification of const/
<?php

class X {
  /** @kphp-const */
  public $v = 123;
}

$x = new X;
$x->v = 2;
