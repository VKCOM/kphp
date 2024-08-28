@ok k2_skip
<?php

/**
 * @kphp-required
 * @param int[] $args
 * @return int
 */
function call_me($args) {
    return count($args);
}


function with_callable(callable $f, ...$args) {
    return $f($args);
}

$res = with_callable('call_me', ...[1, 2, 3]);
var_dump($res);

$res = with_callable(fn($args) => call_me($args), ...[1, 2, 3, 4]);
var_dump($res);

