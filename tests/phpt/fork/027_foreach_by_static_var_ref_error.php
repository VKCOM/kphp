@kphp_should_fail k2_skip
/It is forbidden to call resumable functions inside foreach with reference by var \$static_var_x with non local scope visibility/
<?php

function wait_once($i) {
  static $flag = true;
  if ($flag) {
    wait($i);
    $flag = false;
  }
}

function foo() {
  sched_yield();
  demo(false);
  return null;
}

function demo($x = true) {
  static $static_var_x = [["x"], "y"];

  if (!$x) {
    $static_var_x = 1;
    return;
  }

  $i = fork(foo());
  foreach ($static_var_x as &$value) {
    wait_once($i);
    $value = [1];
  }

  var_dump($static_var_x);
}

demo();
