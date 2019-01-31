@kphp_should_fail
<?php

function f(...$args) {
    var_dump($args);
}

$a = [1, 2, 3];
f(3 + ...$a);
