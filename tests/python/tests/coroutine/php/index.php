<?php

function test_get_running_fork_id() {
  $f = function() {
    // 1. Non-"main" fork. Before std function which is coroutine
    $before = (int)get_running_fork_id();

    sched_yield_sleep(0.01);

    $nested_f = function () {
      // 2. Yet another non-"main" fork. Before std function which is coroutine
      $before = (int)get_running_fork_id();
      sched_yield_sleep(0.01);
      assert_fork_id($before);
      return null;
    };

    assert_fork_id($before);
    $fut = fork($nested_f());
    assert_fork_id($before);
    wait($fut);
    assert_fork_id($before);
    return null;
  };
  $fut = fork($f());

  // 3. "main" fork. Before std function which is coroutine
  $before = (int)get_running_fork_id();
  sched_yield_sleep(0.01);
  assert_fork_id($before);
  wait($fut);
  assert_fork_id($before);
}


function assert_fork_id(int $expected_fork_id) {
  $current_fork_id = (int)get_running_fork_id();
  if ($current_fork_id != $expected_fork_id) {
    critical_error("unexpected fork id, expected: " . $expected_fork_id . "  got: " . $current_fork_id);
  }
}

function main() {
  switch ($_SERVER["PHP_SELF"]) {
    case "/test_get_running_fork_id": {
        test_get_running_fork_id();
        return;
      }
  }

  critical_error("unknown test");
}

main();
