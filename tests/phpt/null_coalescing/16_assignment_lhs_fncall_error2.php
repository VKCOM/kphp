@kphp_should_fail
/Function calls on the left-hand side of \?\?= are not supported/
<?php

class A {
    /** @var ?int */
    public $x = null;
    /** @var A */
    public $next = null;
}

$a = new A();

function get_a(): A {
   global $a;
   return $a;
}

get_a()->next->next->x ??= 99;
