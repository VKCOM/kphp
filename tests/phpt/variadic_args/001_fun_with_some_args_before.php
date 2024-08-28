@ok k2_skip
<?php

/**
 * @param int $x
 * @param int $y
 * @param int[] $args
 */
function f($x, $y, ...$args) {
    var_dump($x);
    var_dump($y);
    var_dump(count($args));
    var_dump($args);
}

f(1, 2, 3, 4, 5, 6);
f(1, 2);
