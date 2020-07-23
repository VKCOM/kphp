@ok
<?php

/**
 * @kphp-infer
 * @param int $x
 * @param mixed[] $args
 */
function f($x, ...$args) {
    $args[0] = 'a';
    var_dump($args);
}

f(1, 2, 3);
