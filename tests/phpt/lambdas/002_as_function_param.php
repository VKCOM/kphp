@ok
<?php

/**
 * @kphp-template $callback
 */
function fun($callback)
{
    var_dump($callback->__invoke(10, 20));
}

$f = function ($x, $y) {
    return $x + $y;
};
fun($f);

$f2 = function ($x, $y) {
    return $x * $y;
};
fun($f2);
