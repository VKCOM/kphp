@kphp_should_fail
<?php

function fun($x = 10, $y = 'a', ...$args) {
    var_dump($x);
    var_dump($y);
    var_dump($args);
}

fun(100, 'b', 1);
