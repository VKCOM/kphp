@kphp_should_fail k2_skip
/Callbacks passed to builtin functions must not be resumable./
<?php

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
    $id = fork(parent_fork());
    $ans = wait($id);
    var_dump($ans);
}

demo();

