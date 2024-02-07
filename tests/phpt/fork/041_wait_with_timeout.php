@ok
<?php
require_once 'kphp_tester_include.php';

function raise_error(string $msg) {
    echo $msg . " " . kphp . "\n";
    die(1);
}

function forkable(int $i, float $sleep_time_sec = 0.5) {
  sched_yield_sleep($sleep_time_sec);
  return $i;
}

function test_several_waits() {
  echo "test_several_waits\n";
  if (kphp) {
    $future = fork(forkable(42));
    var_dump(wait($future, 0.1));
    var_dump(wait($future, 0.2));
    var_dump(wait($future, 0.3));
  } else {
    var_dump(null);
    var_dump(null);
    var_dump(42);
  }
}

function test_nonblocking_wait_not_ready_fork() {
    echo "test_nonblocking_wait_not_ready_fork\n";
    if (kphp) {
      $future = fork(forkable(42, 0.1));
      for ($i = 0; $i < 100; $i++) {
        var_dump(wait($future, 0));
      }
      var_dump(wait($future, 0.2));
    } else {
      for ($i = 0; $i < 100; $i++) {
        var_dump(null);
      }
      var_dump(42);
    }
}

$must_be_unreachable = true;

function control_fork() {
    global $must_be_unreachable;
    sched_yield();
    if ($must_be_unreachable) {
      echo "Must be unreachable " . kphp . "\n";
    }
    return null;
}

function test_nonblocking_wait_not_switch(bool $do_work_until_fork_finishes) {
    global $must_be_unreachable;
    echo "test_nonblocking_wait_not_switch\n";
    if (kphp) {
      $future = fork(forkable(42, 0.1));
      if ($do_work_until_fork_finishes) {
        $x = fork(forkable(-1, 0.2));
        wait($x);
      }

      $control_fork = fork(control_fork());
      $must_be_unreachable = true;
      for ($i = 0; $i < 100; $i++) {
        var_dump(wait($future, 0));
      }
      $must_be_unreachable = false;

      var_dump(wait($future, 0.2));
      wait($control_fork);
    } else {
      for ($i = 0; $i < 100; $i++) {
        if ($do_work_until_fork_finishes && $i === 0) {
          var_dump(42);
        } else {
          var_dump(null);
        }
      }
      if (!$do_work_until_fork_finishes) {
        var_dump(42);
      } else {
        var_dump(null);
      }
    }
}

test_several_waits();
test_nonblocking_wait_not_ready_fork();
test_nonblocking_wait_not_switch(false);
test_nonblocking_wait_not_switch(true);
