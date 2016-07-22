@ok
<?php


define(TIME, 12 * 15 * 16);

/** @kphp-inline */
function net($x) {
  return $x + 2;
}

function i($x) {
  var_dump("i(" . $x . ")");
  static $i = 1;
  global $y;
  return $x + $y + $i;
}

function h($x) {
  var_dump("h(" . $x . ")");
  return $x + 2 * $x;
}

function g($x) {
  // global $e;
  global $q;
  var_dump("g(" . $x . "), q = " . $q);
  $e = i(h(1 + $q)) + $x;
  return $e;
}

/**
* @var $x int
* @kphp-inline
* @kphp-return-value-check int
* @abc
*/
function ff($x = TIME) {
  var_dump($x);
  $s = 0;
  var_dump($s);
  $x = "asd";
  $x = array(0, 1);
  $z = array();
  global $qq;
  $s = $qq[0];
  foreach ($x as $i) {
    $s += $i;
  }
  $s += count($z);
  return g($s);
}

/**
* @kphp-inline
*/

$qqq = 0;
var_dump($qqq);
/*
* @inline
*/
  function f($x) {
    return g($x) * $x;
  }

/** @kphp-inline */
  function e($x) {
    return f(ff(g($x)));
  }


  function z($x) {
    static $iii = 0;
    $iii += 1;
    var_dump($iii);
    global $y;
    $ans1 = e(f($x));
    $ans2 = g(ff($y));    
    return $ans1 + $ans2 + ff();
  }
  $q = 1;
  $qq = 0;
  var_dump($qq);
  $qq = array(1, 2);
  $y = 2;
  var_dump(array(4, 5, 3));
  $z = "x";
  var_dump($z);
  $z = net(10);
  $z = $z + f(3);
  var_dump($z + 1);
  var_dump(z($z));
  var_dump(z(z($z)));
  var_dump($q);
  var_dump(ff(array(55, 22)));
  var_dump(e(15));
  var_dump(ff());
