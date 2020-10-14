@ok
<?php

function callable_lower_f(callable $f) {
    return $f(10);
}

/**
 * @param callable $f
 * @return int
 */
function kphp_callable_f($f) {
    return $f(10);
}

$callable = function ($x) { return $x + 1; }; 
var_dump(callable_lower_f($callable));
var_dump(kphp_callable_f($callable));

