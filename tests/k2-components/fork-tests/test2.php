<?php

function my_fork($level) {
    if ($level > 2) {
        sched_yield();
        return true;
    }
    $left = fork(my_fork($level + 1));
    $right = fork(my_fork($level + 1));
    wait($left);
    wait($right);
    return true;
}

$head = fork(my_fork(0));
wait($head);