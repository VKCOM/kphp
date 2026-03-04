@ok
<?php
require_once 'kphp_tester_include.php';

function task(int $task_id, int $delay_ms): int {
    if ($delay_ms > 0) {
        sched_yield_sleep($delay_ms / 1000);
    }
    return $task_id * 1000 + $delay_ms;
}

function test_wait_queue_negative_timeout(): void {
    var_dump("test_wait_queue_negative_timeout");
    $fork1 = fork(task(1, 50));
    $queue = wait_queue_create([$fork1]);

    $completed = wait_queue_next($queue, -1);
    var_dump(!!$completed);
    if ($completed) {
        var_dump(wait($completed));
    }
}

function test_wait_queue_positive_timeout(): void {
    var_dump("test_wait_queue_positive_timeout");
    $fork1 = fork(task(1, 50));
    $queue = wait_queue_create([$fork1]);

    $completed = wait_queue_next($queue, 0.5);
    var_dump(!!$completed);
    if ($completed) {
        var_dump(wait($completed));
    }
}

function test_wait_queue_zero_timeout_never_waits_returns_result(): void {
    var_dump("test_wait_queue_zero_timeout_never_waits_returns_result");
    $fork1 = fork(task(1, 0));
    $fork2 = fork(task(2, 0));
    $queue = wait_queue_create([$fork1, $fork2]);
    sched_yield();

    $results = [];

    $start = microtime(true);
    $result1 = wait_queue_next($queue, 0);
    $elapsed1 = microtime(true) - $start;

    var_dump(!!$result1);
    if (!!$result1) {
        $results[] = wait($result1);
    }
    var_dump($elapsed1 < 0.01);

    $start = microtime(true);
    $result2 = wait_queue_next($queue, 0);
    $elapsed2 = microtime(true) - $start;

    var_dump(!!$result2);
    if (!!$result2) {
        $results[] = wait($result2);
    }
    var_dump($elapsed2 < 0.01);

    $start = microtime(true);
    $result3 = wait_queue_next($queue, 0);
    $elapsed3 = microtime(true) - $start;

    var_dump(!!$result3);
    if (!!$result3) {
        var_dump(wait($result3));
    }
    var_dump($elapsed3 < 0.01);

    sort($results);
    var_dump($results);
}

function test_wait_queue_zero_timeout_never_waits_returns_null(): void {
    var_dump("test_wait_queue_zero_timeout_never_waits_returns_null");
    $fork1 = fork(task(1, 100_000));
    $fork2 = fork(task(2, 100_000));
    $queue = wait_queue_create([$fork1, $fork2]);

    $start = microtime(true);
    $result = wait_queue_next($queue, 0);
    $elapsed = microtime(true) - $start;

//     php is very smart and was able to get an answer without waiting 100 seconds
//     if (!!$result) {
//         var_dump(wait($result));
//     }

    var_dump($elapsed < 0.01);
}

function test_wait_queue_next(): void {
    var_dump("test_wait_queue_next");
    $fork1 = fork(task(1, 300));
    $fork2 = fork(task(2, 100));
    $fork3 = fork(task(3, 200));
    $queue = wait_queue_create([$fork1, $fork2, $fork3]);

    $results = [];

    while ($completed = wait_queue_next($queue)) {
        $results[] = wait($completed);
    }

    sort($results);
    var_dump($results);
}


test_wait_queue_negative_timeout();
test_wait_queue_positive_timeout();
test_wait_queue_zero_timeout_never_waits_returns_result();
test_wait_queue_zero_timeout_never_waits_returns_null();
test_wait_queue_next();
