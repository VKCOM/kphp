@kphp_should_fail
/Resumable function can.t use die/
<?php

function resumable_f() {
  sched_yield();
  die(1);
}

function test() {
  $x = fork(resumable_f());
  wait($x);
}

test();
