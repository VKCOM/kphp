@ok
<?php

// by default, KPHP_REQUIRE_CLASS_TYPING = 0, in tests also

class A {
  public $a;
}

$a = new A;
$a->a = 1;
$a->a = [1];
