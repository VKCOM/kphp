@ok k2_skip
<?php

/**
 * @param int $x
 * @param int $y
 * @param string[] $args
 */
function fun($x, $y, ...$args) {
    var_dump($x);
    var_dump($y);
    var_dump($args);
}

$a = ['a', 'b', 'c'];
fun(10, 20, ...$a);

fun(10, 20, 'e', 'u', ...$a);
