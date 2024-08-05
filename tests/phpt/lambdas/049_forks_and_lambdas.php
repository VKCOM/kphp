@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

// test that lambdas are forkable

/** @kphp-required */
function justEcho() { echo __FUNCTION__; return null; }

function callFork(callable $f) {
    fork($f());
}

callFork(fn() => print_r(1));
callFork('justEcho');

$xx = 'xx';
callFork(function() use($xx) { echo $xx; return null; });

// test resumable and complex expressions (tmp vars) inside lambdas

function interrupting() {
    sched_yield();
    return 1;
}

function acceptCallback(callable $f) {
    $bb = $f() ?? 1;
    return null;
}

/** @kphp-required */
function passedAsString(): ?int {
    $a = interrupting() ?? 1;
    return 0;
}

function demo1() {
    return (function() {
        return interrupting() ?? 3;
    })() ?? 0;
}

function demo2() {
    $b = acceptCallback('passedAsString') ?? 2;
    return null;
}

fork(demo1());
fork(demo2());


