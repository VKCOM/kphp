@ok
<?php

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
}


function g($id) {
  global $f_resumable; 
  $res = wait($f_resumable);
  var_dump("g", $id, $res);
}

function h($id) {
  global $f_resumable, $f_finished; 
  $res = wait_multiple($f_resumable);
  $res = true;
  var_dump("h", $id, $res);
}


$f_resumable = fork(f());

$gid1 = fork(g(0));
$gid2 = fork(g(1));

wait($gid1);
wait($gid2);

$f_resumable = fork(f());

$hid1 = fork(h(0));
$hid2 = fork(h(1));

wait($hid1);
wait($hid2);
