@ok UB no_php
<?
  $a = array (0 => 1, 1 => 2);

  function f (&$x) {
    global $a;
    foreach ($a as $k => $v) {
      $x = 3;
      var_dump ($v);
    }
  }

  f ($a[1]);