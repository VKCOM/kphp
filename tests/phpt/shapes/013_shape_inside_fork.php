@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @return shape(s:string, int:int)
 */
function getT() {
    sched_yield();
    return shape(['int' => 1, 's' => 'string']);
}

/**
 * @return shape(1:int, a:\Classes\A, s:string, arr:int[], arr_a:\Classes\A[], first:int|false)
 */
function getBigShape() {
    $a = 1 ? 1 : false;
    sched_yield();
    return shape(['first' => $a, '1' => 1, 'arr' => [1,2,3], 'a' => new Classes\A, 's' => 's', 'arr_a' => [new Classes\A]]);
}

/**
 * @param bool $getFalse
 * @return shape(1:int, a:\Classes\A, s:string, arr:int[], arr_a:\Classes\A[], first:int|false)|false
 */
function getTOrFalse($getFalse) {
    if($getFalse)
        return false;

    return getBigShape();
}

function demo() {
    $t1 = getT();
    echo $t1['int'], ' ', $t1['s'], "\n";

    $t2 = getBigShape();
    echo $t2['first'], ' ', $t2['s'], "\n";

    if($t3 = getTOrFalse(false)) {
        echo $t3['first'], ' ', $t3['s'], "\n";
    } else {
        echo "false", "\n";
    }

    if($t4 = getTOrFalse(true)) {
        echo $t4['first'], ' ', $t4['s'], "\n";
    } else {
        echo "false", "\n";
    }

    return null;
}

$ff = fork(demo());
wait($ff);
