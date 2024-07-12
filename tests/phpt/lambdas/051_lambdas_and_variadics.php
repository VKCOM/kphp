@ok k2_skip
<?php

/**
 * @kphp-required
 * @param int[] $args
 */
function call_me_variadic(...$args) {
    return count($args);
}

function with_callable_variadic(callable $f, ...$args) {
    return $f(...$args);
}

$extern_x_1 = 11;

var_dump(with_callable_variadic('call_me_variadic', 1, 2, 3));
var_dump(with_callable_variadic(fn(...$args) => call_me_variadic(...$args), 1, 2, 3));
var_dump(with_callable_variadic(fn(...$args) => call_me_variadic(10, ...$args), 1, 2, 3));
var_dump(with_callable_variadic(fn(...$args) => call_me_variadic(10, $extern_x_1, ...$args), 1, 2, 3));


/**
 * @param (callable(int):int) ...$callbacks
 */
function takeMultiCallbacks(int $arg, ...$callbacks) {
    foreach ($callbacks as $cb)
        echo $cb($arg), "\n";
}


takeMultiCallbacks(1,
    fn($x) => $x,
    fn($x) => $x + 1,
    fn($x) => $x * 2,
);
