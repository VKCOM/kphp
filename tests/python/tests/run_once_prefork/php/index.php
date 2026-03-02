<?php

function suspend_($time) {
    sched_yield_sleep($time);
    return null;
}

function suspend($time) {
    $f = fork(suspend_($time));
    wait($f);
}

echo "PID: " . posix_getpid() . " - before suspend\n";
suspend(0.001);
echo "Done - after suspend\n";
