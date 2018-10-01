@ok
<?php

$fun = function ($x, $y) {
    return $x + $y;
};
var_dump($fun->__invoke(10, 20));
