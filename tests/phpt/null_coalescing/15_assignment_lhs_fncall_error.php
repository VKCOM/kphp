@kphp_should_fail
/Function calls on the left-hand side of \?\?= are not supported/
<?php

class A {
    /** @var ?int */
    public $x = null;
}

function get_a(): A {
   return new A();
}

get_a()->x ??= 99;
