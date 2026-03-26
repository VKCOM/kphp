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
    // kphp_backtrace doesn't capture coro trace after coroutine suspending
    $backtrace[] = ["function" => "coro_fun3"];
    $backtrace[] = ["function" => "coro_fun2"];
    $backtrace[] = ["function" => "coro_fun1"];
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
    $res = sync_fun1($x ? count($x) : -1);
    return $res;
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

function test_backtrace() {
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
                $f_2 = fork(coro_fun3([]));

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

function make_custom_exception() {
    return err('TEST', 'London is the capital of Great Britain');
}

function check_exception(Exception $e) {
    if ($e->getFile() != 'index.php')
        return 'expected "index.php" but "' . $e->getFile() . '" found';

    if ($e->getLine() != 113)
        return 'expected 113 but ' . $e->getLine() . ' found';

    if ($e->getCode() != 0)
        return 'expected 0 but ' . $e->getCode() . ' found';

    if ($e->getMessage() != 'ERR_TEST: London is the capital of Great Britain')
        return 'expected "ERR_TEST: London is the capital of Great Britain" but "' . $e->getMessage() . '" found';

    return 'ok';
}

function test_err() {
    foreach (json_decode(file_get_contents('php://input')) as $action) {
        switch ($action["op"]) {
            case "just_make":
                $e = make_custom_exception();
                $res = check_exception($e);
                echo $res;
                break;
            case "throw_catch":
                try {
                    throw make_custom_exception();
                } catch (Exception $e) {
                    $res = check_exception($e);
                }
                echo $res;
                break;
            default:
                echo "unknown operation";
                return;
        }
    }
}

function main() {
    switch ($_SERVER["PHP_SELF"]) {
        case "/test_backtrace":
            test_backtrace();
            break;
        case "/test_err":
            test_err();
            break;
        default:
            critical_error("unknown test");
    }
}

main();

