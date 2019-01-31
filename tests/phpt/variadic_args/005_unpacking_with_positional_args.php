@ok
<?php

function fun($x, $y, ...$args) {
    var_dump($x);
    var_dump($y);
    var_dump($args);
}

$a = ['a', 'b', 'c'];
fun(10, 20, ...$a);

fun(10, 20, 'e', 'u', ...$a);
