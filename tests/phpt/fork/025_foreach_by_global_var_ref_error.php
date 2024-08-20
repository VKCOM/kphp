@kphp_should_fail k2_skip
/It is forbidden to call resumable functions inside foreach with reference by var \$global_var_x with non local scope visibility/
<?php

$global_var_x = [["x"], "y"];

function wait_once($i) {
  static $flag = true;
  if ($flag) {
    wait($i);
    $flag = false;
  }
}

function foo() {
  global $global_var_x;
  sched_yield();
  $global_var_x = 1;
  return null;
}

function demo() {
  global $global_var_x;

  $i = fork(foo());
  foreach ($global_var_x as &$value) {
    wait_once($i);
    $value = [1];
  }

  var_dump($global_var_x);
}

demo();
