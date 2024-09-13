@ok k2_skip
<?php

/**
 * @return int
 */
function f() {
    return 10;
    array_map(function ($x) { return $x; }, [1, 2, 3]);
    $f = function ($y) { return $y + 2; };
}

f();
