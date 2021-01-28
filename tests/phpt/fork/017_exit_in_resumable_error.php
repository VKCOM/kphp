@kphp_should_fail
/Resumable function can.t use exit/
<?php

function resumable_f() {
  sched_yield();
  exit(1);
}

function test() {
  $x = fork(resumable_f());
  wait($x);
}

test();
