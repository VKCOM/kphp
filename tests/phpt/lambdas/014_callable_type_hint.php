@ok
<?php

function callable_upper_f(Callable $f) {
    return $f(10);
}

function callable_lower_f(callable $f) {
    return $f(10);
}

/**
 * @kphp-infer hint check
 * @param Callable $f
 */
function kphp_callable_f(Callable $f) {
    return $f(10);
}

$callable = function ($x) { return $x + 1; }; 
var_dump(callable_upper_f($callable));
var_dump(callable_lower_f($callable));
var_dump(kphp_callable_f($callable));

