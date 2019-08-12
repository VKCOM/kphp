@ok
<?php
#ifndef KittenPHP
require_once __DIR__.'/../dl/polyfill/fork-php-polyfill.php';
require_once 'polyfills.php';
require_once 'Classes/autoload.php';
#endif

function getT() {
    sched_yield();
    return tuple(1, 'string');
}

function getBigTuple() {
    $a = 1 ? 1 : false;
    return tuple($a, 1, [1,2,3], new Classes\A, 's', [new Classes\A]);
}

function getTOrFalse($getFalse) {
    if($getFalse)
        return false;

    return getBigTuple();
}

function demo() {
    $t1 = getT();
    echo $t1[0], ' ', $t1[1], "\n";
    echo count($t1), "\n";

    $t2 = getBigTuple();
    echo $t2[0], ' ', $t2[4], "\n";
    echo count($t2), "\n";

    if($t3 = getTOrFalse(false)) {
        echo $t3[0], ' ', $t3[4], "\n";
        echo count($t3), "\n";
    } else {
        echo "false", "\n";
    }

    if($t4 = getTOrFalse(true)) {
        echo $t4[0], ' ', $t4[4], "\n";
        echo count($t4), "\n";
    } else {
        echo "false", "\n";
    }
}

$ii = fork(demo());
wait_result($ii);
