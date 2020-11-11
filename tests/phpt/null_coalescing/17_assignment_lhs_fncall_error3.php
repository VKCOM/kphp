@kphp_should_fail
/Function calls on the left-hand side of \?\?= are not supported/
<?php

class A {
    /** @var ?int */
    public $x = null;
}

function f() : int { return 0; }

$a = [new A()];

$a[f()]->x ??= 99;
