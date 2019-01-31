@kphp_should_fail
<?php

function fun($x, ...$args) {
    var_dump($x);
    var_dump($args);
}

$a = [1, 2, 3];
fun(...$a, ...$a);
