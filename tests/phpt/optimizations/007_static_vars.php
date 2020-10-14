@ok
<?php

/**
 * @param int $x
 */
function foo($x) {
    static $st = 99;

    $st++;
    if ($x == 100) {
        var_dump($x);
    } else {
        var_dump($x);
        foo($st);
    }
}

foo(10);
