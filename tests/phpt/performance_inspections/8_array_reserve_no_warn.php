@ok k2_skip
KPHP_ERROR_ON_WARNINGS=1
<?php

require_once 'kphp_tester_include.php';

$y2 = [];

class A {
  public $y3 = [];
}
/**
 * @kphp-warn-performance array-reserve
 */
function test1() {
  global $y2;

  $y1 = ["hello" => []];
  for ($i = 0; $i != 1000; ++$i) {
    $y1["hello"][] = $i + 2;
  }

  foreach ($y1 as $v) {
    $y2[] = $v;
  }

  $a = new A;
  foreach ($y2 as $k => $v) {
    $a->y3[$k] = $v;
  }

  $y4 = [];
  $i = 1;
  while ($i < 1000) {
    $y4[] = ++$i;
    if ($i % 2) {
      break;
    }
  }

  $y5 = [];
  do {
    if ($i % 3) {
      $y5[] = $i;
    }
  } while ($i--);
}

/**
 * @kphp-warn-performance array-reserve
 */
function test2() {
  $y1 = [];
  array_reserve($y1, 1000, 0, true);
  for ($i = 0; $i != 1000; ++$i) {
    $y1[] = $i + 2;
  }

  $y2 = [];
  array_reserve_from($y2, $y1);
  foreach ($y1 as $v) {
    $y2[] = $v;
  }

  $y3 = [];
  array_reserve_map_int_keys($y3, count($y2));
  foreach ($y2 as $k => $v) {
    $y3[$k] = $v;
  }

  $y4 = [];
  $i = 1;
  array_reserve_map_string_keys($y4, 1000);
  while ($i < 1000) {
    $y4["$i xx"] = ++$i;
  }

  $y5 = [];
  array_reserve_vector($y5, $i);
  do {
    $y5[] = $i;
  } while ($i--);
}

test1();
test2();
