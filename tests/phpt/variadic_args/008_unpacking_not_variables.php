@ok k2_skip
<?php

/**
 * @param mixed $x
 * @param mixed[] $args
 */
function fun($x, ...$args) {
    var_dump($x);
    var_dump($args);
}

/**
 * @param mixed[] $args
 * @return mixed[]
 */
function get_arr(...$args) {
    $args[] = 'd';
    return $args;
}

fun(10, ...[1, 2, 3]);
fun('a', 'a', get_arr(['z']), ...get_arr(...get_arr('b', ...['c'])));
