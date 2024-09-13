@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

$a = array (0 => 1, 1 => 2);

function f () {
  global $id;

  wait($id);
}

function i () {
  sched_yield();
  sched_yield();
  sched_yield();
  sched_yield();
  return null;
}

function g () {
  global $a;
  sched_yield();

  $aa = $a;
  var_dump ($aa);
  for ($k = 0; $k < 2; $k++) {
    $v = $aa[$k];
    var_dump ($v);
    usleep (10000);
    sched_yield();
  }
  return null;
}

function h (&$x) {
  global $id3;
  sched_yield();
  sched_yield();
  wait($id3, 0.001);
  $x = 6;
  sched_yield();
}


$id3 = fork (i());
$id = fork (g());

f ();
