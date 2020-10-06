@ok
<?php

function test_or_false_assign() {
  $xyz1 = true ? ["hello", "world"] : false;
  list($x1, $y1, $z1, ) = $xyz1;
  var_dump($x1);
  var_dump($y1);
#ifndef KPHP
  if (is_null($z1)) { $z1 = ""; }
#endif
  var_dump($z1);

  $xyz2 = false ? ["hello", "world"] : false;
  list($x2, $y2, $z2, ) = $xyz2;
#ifndef KPHP
  if (is_null($x2)) { $x2 = ""; }
  if (is_null($y2)) { $y2 = ""; }
  if (is_null($z2)) { $z2 = ""; }
#endif
  var_dump($x2);
  var_dump($y2);
  var_dump($z2);
}

function test_var_assign() {
  $xyz1 = true ? ["hello", "world"] : "hello world";
  list($x1, $y1, $z1, ) = $xyz1;
  var_dump($x1);
  var_dump($y1);
  var_dump($z1);

  $xyz2 = false ? ["hello", "world"] : "hello world";
  list($x2, $y2, $z2, ) = $xyz2;
#ifndef KPHP
  $x2 = "h";
  $y2 = "e";
  $z2 = "l";
#endif
  var_dump($x2);
  var_dump($y2);
  var_dump($z2);
}

test_or_false_assign();
test_var_assign();
