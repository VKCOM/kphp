@ok
<?php
  function test ($a, $b) {
    var_dump ($a + $b);
    var_dump ($b + $a);
    $c = $b;
    var_dump ($b += $a);
    var_dump ($a += $c);
  }

  $a = array (1, 2, 3);
  $b = array (4, 5, 6, 7, 8);
  $c = array ("a" => 9, "b" => 10, 3 => 11, 1 => 12);
  $d = array ("a" => 13, "c" => 14, 4 => 15, 1 => 16, -70 => 213123, 234234, 234234, 23423434, 3453, 45, 34, 534, 5, 345, 34, 534, 5, 345, 34, 534, 5, 345);
  $e = $a + $c;
  $f = $d + $b;

  var_dump ($a);
  var_dump ($b);
  var_dump ($c);
  var_dump ($d);
  var_dump ($e);
  var_dump ($f);

  test ($a, $a);
  test ($a, $b);
  test ($a, $c);
  test ($a, $d);
  test ($a, $e);
  test ($a, $f);

  test ($b, $a);
  test ($b, $b);
  test ($b, $c);
  test ($b, $d);
  test ($b, $e);
  test ($b, $f);

  test ($c, $a);
  test ($c, $b);
  test ($c, $c);
  test ($c, $d);
  test ($c, $e);
  test ($c, $f);

  test ($d, $a);
  test ($d, $b);
  test ($d, $c);
  test ($d, $d);
  test ($d, $e);
  test ($d, $f);

  test ($e, $a);
  test ($e, $b);
  test ($e, $c);
  test ($e, $d);
  test ($e, $e);
  test ($e, $f);

  test ($f, $a);
  test ($f, $b);
  test ($f, $c);
  test ($f, $d);
  test ($f, $e);
  test ($f, $f);

  var_dump ($a);
  var_dump ($b);
  var_dump ($c);
  var_dump ($d);
  var_dump ($e);
  var_dump ($f);
