@kphp_should_fail
/Modification of const/
<?php

class X {
    /**
     * @kphp-infer
     * @param X|false $x
     */
  function __construct($x) {
    if ($x) {
      $x->v = 10;
    }
  }

  /** @kphp-const */
  public $v = 123;
}

$x = new X(false);
$xx = new X($x);
