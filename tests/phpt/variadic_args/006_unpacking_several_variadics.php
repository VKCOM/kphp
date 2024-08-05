@ok k2_skip
<?php

/**
 * @param int $x
 * @param mixed[] $args
 */
function fun($x, ...$args) {
    var_dump($x);
    var_dump($args);
}

$a = ['a', 'b', 'c'];
$b = [['i', 'o'], 10, ['0']];
fun(10, 10, [1, 2, 3], ...$a, ...$b);
