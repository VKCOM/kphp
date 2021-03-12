@kphp_should_fail
KPHP_REQUIRE_CLASS_TYPING=1
/Specify @var or type hint or default value to A::\$a/
<?php

// when class typing required, use @var or default value

class A {
  public $a;
}

$a = new A;
$a->a = 1;
$a->a = [1];
