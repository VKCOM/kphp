@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

/**
 * @kphp-required
 **/
function child_fork(int $i): int {
    sched_yield_sleep(0.001);
    return $i * 100;
}

function parent_fork() {
    $arr = [];
    for ($i = 0; $i < 10; $i++) {
        $arr[] = $i;
    }
    $ans = array_map('child_fork', $arr);
    return $ans[5];
}


function demo() {
    var_dump(parent_fork());
}

demo();
