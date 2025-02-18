@ok
<?php
require_once 'kphp_tester_include.php';

function fork_function() {
  sched_yield_sleep(2);
  return 777;
}

function awaiter1($fut) {
  return wait_concurrently($fut);
}

function awaiter2($fut) {
  return wait($fut);
}

function awaiter3($fut) {
  return wait_concurrently($fut);
}

$fut = fork(fork_function());
$x2 = fork(awaiter2($fut));
$x1 = fork(awaiter1($fut));
$x3 = fork(awaiter3($fut));

#ifndef KPHP
  var_dump(true);
  var_dump(777);
  var_dump(true);
if (false)
#endif
{
var_dump(wait($x1));
var_dump(wait($x2));
var_dump(wait($x3));
}
