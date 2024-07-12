@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

$f_finished = false;

function f() {
  echo "f 1\n";
  sched_yield();
  echo "f 2\n";
  sched_yield();
  echo "f 3\n";
  sched_yield();
  echo "f 4\n";
  $f_finished = true;
  return $f_finished;
}

/**
 * @param int $id
 */
function g($id) {
  global $f_resumable; 
  wait($f_resumable);
  $res = true;
  var_dump("g", $id, $res);
  return null;
}

/**
 * @param int $id
 */
function h($id) {
  global $f_resumable, $f_finished; 
  $res = wait_concurrently($f_resumable);
  $res = true;
  var_dump("h", $id, $res);
  return null;
}

echo "-----------<stage 1>-----------\n";

#ifndef KPHP
  echo "f 1\n";
  var_dump("g", 1, true);
  echo "f 2\n";
  echo "f 3\n";
  echo "f 4\n";
  var_dump("g", 0, true);
if (false)
#endif
{
$f_resumable = fork(f());

$gid1 = fork(g(0));
$gid2 = fork(g(1));

wait($gid1);
wait($gid2);
}
echo "-----------<stage 2>-----------\n";

$f_resumable = fork(f());

$hid1 = fork(h(0));
$hid2 = fork(h(1));

wait($hid1);
wait($hid2);
