<?php


function func_with_one_suspend_point($id) {
    warning("start func with one suspend point ". $id);

    warning("sched yield " .$id);
    sched_yield();
    warning("resume " .$id);
    warning("finish " .$id);
    return true;
}

$futures = [];

for ($i = 1; $i <= 5; $i++) {
    $futures[] = fork(func_with_one_suspend_point($i));
}

warning("start wait forks");
for ($i = 4; $i >= 0; $i--) {
    wait($futures[$i]);
}