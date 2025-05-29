<?php

function my_array_find($arr, callable $callback) {
    foreach($arr as $element) {
        if ($callback($element)) {
            return $element;
        }
    }
    return null;
}

function capture_backtrace() {
#ifndef K2
    $trace = kphp_backtrace();
    $backtrace = [];
    foreach($trace as $function) {
        $backtrace[] = ["function" => $function];
    }
    return $backtrace;
#endif
    return debug_backtrace();
}


function check_backtrace($backtrace) {
    $ok = true;
    $trace = ["sync_fun2", "sync_fun1", "coro_fun3", "coro_fun2", "coro_fun1"];

    foreach ($trace as $symbol) {
        $ok &= my_array_find($backtrace, function(array $value) use($symbol) {
            return stristr($value["function"], $symbol) != false;
            }) != null;
        if (!$ok) {
            return $symbol . " is absent";
        }
    }

    return "ok";
}


function sync_fun2(bool $b) {
    while(0);
    $trace = capture_backtrace();
    return check_backtrace($trace);
}

function sync_fun1(int $x) {
    while(0);
    return sync_fun2($x > 0);
}

function coro_fun3(?array $x) {
    while(0);
    sched_yield_sleep(0.01);
    return sync_fun1($x ? count($x) : -1);
}

function coro_fun2(array $x) {
    while(0);
    return coro_fun3(false ? $x : null);
}

function coro_fun1() {
    while(0);
    return coro_fun2([1, 2, 3, 4, 5, 6, 7]);
}

function forkable() {
    $f = fork(coro_fun1());
    return wait($f);
}

function main() {
    foreach (json_decode(file_get_contents('php://input')) as $action) {
        switch ($action["op"]) {
            case "check_main_trace":
                $res = coro_fun1();
                echo $res;
                break;
            case "check_fork_trace":
                $f = fork(coro_fun1());
                $res = wait($f);
                echo $res;
                break;
            case "check_fork_switch_trace":
                $f_1 = fork(coro_fun1());
                $f_2 = fork(coro_fun1());

                $res_1 = wait($f_1);
                $_ = wait($f_2);

                echo $res_1;
                break;
            case "check_nested_fork_trace":
                $f = fork(forkable());
                $res = wait($f);
                echo $res;
                break;
            default:
                echo "unknown operation";
                return;
        }
    }
}

main();

