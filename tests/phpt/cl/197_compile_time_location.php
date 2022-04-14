@ok
<?php

require_once 'kphp_tester_include.php';

function f(CompileTimeLocation $loc = null) {
    $loc = CompileTimeLocation::calculate($loc);
    $basename = basename($loc->file);
    echo "f() called from ", $basename, ' ', $loc->function, ':', $loc->line, "\n";
}

function log_info(string $message, CompileTimeLocation $loc = null) {
    $loc = CompileTimeLocation::calculate($loc);
    $basename = basename($loc->file);
    echo "$message (in {$loc->function} at {$basename}:{$loc->line})\n";
}

function my_log_more(string $message, ?CompileTimeLocation $loc = null) {
    $loc = CompileTimeLocation::calculate($loc);
    log_info("!!" . $message, $loc);
}

function demo1() {
    f();
    f();
}

class A {
    static function demo2() {
        f();
    }

    function demo3() {
        f();
    }
}

function demo4() {
    (function() {
        f();
    })();
}

function demo5() {
    log_info("start calculation");
    $arr = [1,2,3,4,5];
    foreach ($arr as $v) {
        if ($v > 3)
            log_info("big v=$v");
    }
    log_info("end calculation");
}

function demo6() {
    my_log_more("start calculation");
    $arr = [1,2,3,4,5];
    foreach ($arr as $v) {
        if ($v > 3)
            my_log_more("big v=$v");
    }
    my_log_more("end calculation");
}


demo1();
A::demo2();
(new A)->demo3();
demo4();
f();
demo5();
demo6();
my_log_more('end');
