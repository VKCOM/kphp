@ok
<?php

/**
 * @kphp-required
 */
function call_me(...$args) {
    return count($args);
}


function with_callable(callable $f, ...$args) {
    return $f(...$args);
}

$res = with_callable('call_me', 1, 2, 3);
var_dump($res);


