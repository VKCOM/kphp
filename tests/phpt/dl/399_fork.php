@wa no_php
<?php
  $a = array (0 => 1, 1 => 2);

  function f () {
    global $id;

    wait ($id);
  }

  function i () {
    sched_yield();
    sched_yield();
    sched_yield();
    sched_yield();
  }

  function g () {
    sched_yield();
    global $a;

    $aa = $a;
    var_dump ($aa);
    for ($k = 0; $k < 2; $k++) {
      $v = $aa[$k];
      var_dump ($v);
      usleep (10000);
      sched_yield();
    }
  }

  function h (&$x) {
    sched_yield();
    sched_yield();
    global $id3;
    wait ($id3, 0.001);
    $x = 6;
    sched_yield();
  }


  $id3 = fork (i());
  $id = fork (g());
  $id2 = fork (h($a[1]));

  f ();
