@ok UB
<?
  $a = array (0 => 1, 1 => 2);

  /**
   * @kphp-infer
   * @param int $x
   */
  function f (&$x) {
    global $a;
    foreach ($a as $k => $v) {
      $x = 3;
      var_dump ($v);
    }
  }

  f ($a[1]);
