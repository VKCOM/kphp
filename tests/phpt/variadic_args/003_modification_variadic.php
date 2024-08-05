@ok k2_skip
<?php

/**
 * @param int $x
 * @param mixed[] $args
 */
function f($x, ...$args) {
    $args[0] = 'a';
    var_dump($args);
}

f(1, 2, 3);
