@ok k2_skip
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
require_once 'kphp_tester_include.php';
#endif

/** @return Classes\A */
function getA() {
    sched_yield();
    $a = new Classes\A;
    $a->a = 22;
    return $a;
}

/** @return Classes\A [] */
function getAArr() {
    sched_yield();
    $a1 = new Classes\A;
    $a1->a = 10;
    return [$a1, new Classes\A];
}

function demo() {
    $a = new Classes\A;
    echo $a->a, "\n";

    $a2 = getA();
    echo $a2->a, "\n";

    $arr = getAArr();
    foreach($arr as $aa) {
        echo $aa->a, "\n";
    }
    return null;
}

$ii = fork(demo());
wait($ii);
