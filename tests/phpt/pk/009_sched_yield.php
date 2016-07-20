@ok no_php
<?php


function f() {
  global $gid;
  $id = $gid++;
  for ($i = 0; $i <= 10; $i++) {
    sched_yield();
    echo "f $id $i\n";
  }
}

$last = false;

for ($i = 0; $i <= 1000; $i++) {
  $last = fork(f());
}

wait($last);
