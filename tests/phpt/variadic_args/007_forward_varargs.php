@ok k2_skip
<?php

/**
 * @param int $x
 * @param mixed[] $args
 */
function get_forwarded($x, ...$args) {
    var_dump($x);
    var_dump($args);
}

/**
 * @param string $y
 * @param int[] $args
 */
function pass_to($y, ...$args) {
    return get_forwarded($args[0], $y, ...$args);
}

$arr = [1, 2, 3];
$arr2 = [4];
pass_to('a', ...$arr2, ...$arr);
