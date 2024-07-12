@kphp_should_fail k2_skip
/It is forbidden to call resumable functions inside foreach with reference by var A::\$x with non local scope visibility/
<?php

class A {
  /** @var mixed */
  public $x = [["x"], "y"];
}

function wait_once($i) {
  static $flag = true;
  if ($flag) {
    wait($i);
    $flag = false;
  }
}

function foo(A $a) {
  sched_yield();
  $a->x = 1;
  return null;
}

function demo() {
  $a = new A;

  $i = fork(foo($a));
  foreach ($a->x as &$value) {
    wait_once($i);
    $value = [1];
  }

  var_dump($a->x);
}

demo();
