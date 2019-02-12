@kphp_should_fail
<?php

class A { public $i = 10; }

function f() {
    $x = new A();
    return $x;
}

$a = f();
var_dump((clone $a)->i);

