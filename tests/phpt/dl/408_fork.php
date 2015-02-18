@ok no_php
<?php

  function g ($x) {
    switch ($x) {
      case 0:
        $mul = g (-1);
        return 100;
      case 1:
        return 200;
      case 2:
        return 300;
      case 3:
        return 400;
      case 4:
        return 500;
      case 5:
        return 600;
      case 6:
        return 700;
      case 7:
        return 800;
      case 8:
        return 900;
      case 9:
        return 1000;
      case 10:
        return 1100;
      case 11:
        return 1200;
      case 12:
        return 1300;
      case 13:
        return 1400;
      case 14:
        return 1500;
      case 15:
        return 1600;
      case 16:
        return 1700;
      case 17:
        return 1800;
      case 18:
        return 1900;
      case 19:
        return 2000;
      case 20:
        return 2100;
      case 21:
        return 2200;
      case 22:
        return 2300;
      case 23:
        return 2400;
      case 24:
        return 2500;
      case 25:
        return 2600;
      case 26:
        return 2700;
    }
  }

  for ($i = -1; $i < 30; $i++) {
    var_dump (wait_result (fork (g ($i))));
  }

  $q = wait_queue_create (array ());
  for ($i = -1; $i < 30; $i++) {
    wait_queue_push ($q, fork (g ($i)));
  }
  while ($t = wait_queue_next ($q)) {
    $res[] = wait_result ($t);
  }
  sort ($res);
  var_dump ($res);

