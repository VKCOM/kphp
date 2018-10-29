@ok
<?php

require_once("Classes/autoload.php");

/**
 * @kphp-template $arg
 */
function HHHH($arg) {
    var_dump($arg->magic);
    return $arg;
}

/**
 * @kphp-template T $arg
 * @kphp-template $arg2
 * @kphp-return T
 */
function GGGG($arg, $arg2) {
    var_dump(count($arg2));
    return HHHH($arg);
}

function FFFF() {
    $b = new Classes\B;
    $b->magic = 100;
    $x = GGGG($b, [$b]);
    $y = GGGG($x, [1]);

    var_dump($y->magic);

    $a = new Classes\A;
    $a->magic = 200;
    GGGG($a, [$a]);
}


FFFF();


