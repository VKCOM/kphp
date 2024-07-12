@ok k2_skip
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
require_once 'kphp_tester_include.php';
#endif

/**
 * @return tuple(int, string)
 */
function getT() {
    sched_yield();
    return tuple(1, 'string');
}

/**
 * @return tuple(int|false, int, int[], \Classes\A, string, \Classes\A[])
 */
function getBigTuple() {
    $a = 1 ? 1 : false;
    return tuple($a, 1, [1,2,3], new Classes\A, 's', [new Classes\A]);
}

/**
 * @param bool $getFalse
 * @return tuple(int|false, int, int[], \Classes\A, string, \Classes\A[])|false
 */
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
    return null;
}

$ii = fork(demo());
wait($ii);
