@kphp_should_fail
/assign array< int > to A::\$a/
<?php

// if KPHP_REQUIRE_CLASS_TYPING = 0, when @var is set, it is checked

class A {
  /** @var int */
  public $a = 0;
}

$a = new A;
$a->a = [1];
