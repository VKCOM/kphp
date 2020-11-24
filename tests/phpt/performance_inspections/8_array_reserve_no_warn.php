@ok
KPHP_ERROR_ON_WARNINGS=1
<?php

$y2 = [];

class A {
  public $y3 = [];
}
/**
 * @kphp-warn-performance array-reserve
 */
function test() {
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

test();
