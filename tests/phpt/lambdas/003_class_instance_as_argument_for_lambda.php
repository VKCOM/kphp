@ok
<?php

require_once("polyfills.php");

/**
 * @kphp-template $callback
 */
function fun($callback)
{
    $a = new Classes\IntHolder();
    $res = $callback->__invoke($a);
    var_dump($res);

    $b = new Classes\AnotherIntHolder();
    $res = $callback->__invoke($b);
    var_dump($res);
}

$f = function ($a) {
    return $a->a + 10;
};
fun($f);

$f2 = function ($a) {
    return $a->a + 10000;
};
fun($f2);


