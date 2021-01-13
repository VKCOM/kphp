<?php

function sleep_for_net_time() {
  sched_yield_sleep(0.01);
  return null;
}

function wait_for_script_time() {
  $t = microtime(true) + 0.01;
  while($t > microtime(true)) {}
}

function run() {
  wait_for_script_time();

  $i1 = fork(sleep_for_net_time());
  wait($i1);
}

run();
echo "Hello world!";
