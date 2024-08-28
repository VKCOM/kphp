@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

/**
 * @param int $t
 */
function my_usleep ($t): bool {
  $end_time = microtime (true) + $t * 1e-6;
  while (microtime (true) < $end_time) {
    sched_yield();
    usleep (min (($end_time - microtime (true)) * 1e6 + 1000, 77777));
  }
  return true;
}

$x = fork (my_usleep (3000000));

/**
 * @param future<bool> $x
 */
function wait_x ($x): bool {
  $ok = true;
  for ($i = 0; $i < 6; $i++) {
    $res_tmp = wait($x);
    $res = $res_tmp != null;
    var_dump("i = $i; res = $res");
  }
  return true;
}

wait_x ($x);

function run () {
  $y = fork (my_usleep (1000000));
  $z = fork (wait_x ($y));
  $t = wait($z);
  $t = $t != null;
  return null;
}

run();

wait(fork (run()));
