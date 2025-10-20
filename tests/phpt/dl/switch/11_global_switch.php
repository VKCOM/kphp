@ok
<?php

  switch (intval ($x)) {
    case 0:
      var_dump (++$x);
      require '11_global_switch.php';
      break;
    case 1:
    case 2:
      var_dump ($x++);
      var_dump ($x++);

      if ($y == 1) {
        var_dump ("6");
        require '11_global_switch.php';
        break;
      } else {
        var_dump ("5");
        require '11_global_switch.php';
      }
    case 10:
      var_dump ("7");
      break;
    case 3:
      $y = 1;
      var_dump ($x--);
      require '11_global_switch.php';
      break;
    default:
      var_dump ($x++);
      break;
  }
