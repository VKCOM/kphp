@kphp_should_fail k2_skip
/It is forbidden to call resumable functions inside foreach with reference by var A::\$static_var_x with non local scope visibility/
<?php

class A {
  static $static_var_x = [["x"], "y"];
}

function wait_once($i) {
  static $flag = true;
  if ($flag) {
    wait($i);
    $flag = false;
  }
}

function foo() {
  sched_yield();
  A::$static_var_x = 1;
  return null;
}

function demo() {
  $i = fork(foo());
  foreach (A::$static_var_x as &$value) {
    wait_once($i);
    $value = [1];
  }

  var_dump(A::$static_var_x);
}

demo();
