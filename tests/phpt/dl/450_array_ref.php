@ok
<?php

function f($x, &$y) {
  $y = 1;
  var_dump ($x);
}

function g(&$y, $x) {
  $y = 1;
  var_dump ($x);
}

//  f($a, $a[0]);
//  g($b[0], $b);

$c = array (3, 4);

foreach ($c as &$xx) {
  $d = $c;
  $xx = 5;
#ifndef KittenPHP
  $d = [$c[0], $c[1]];
#endif
  var_dump ($d);
}

//  var_dump (1e);

function s() {
  return 2;
//    t(1);
  $x = __LINE__.'';
}

function t($x) {
  return $x;
}

//  s();

if (false)
for (;'1';) {}

$cur_feed_types_raw = ",";
$cur_feed_types_raw = ($cur_feed_types_raw == '*' ? true : array ());

$a = null; if (isset ($a)) {}

$tt = isset ($a);

$x = (true ? true : false);
$x = (true ? true : false);
$x = (true ? true : false);

$y = 1;
// $x = &$y;


$z = null;
if (0)
  $z = false;

$x = (true ? null : $z);
