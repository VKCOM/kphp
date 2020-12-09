@kphp_should_warn
/variable \$y1 can be reserved with array_reserve or array_reserve_from out of loop/
/variable \$y2 can be reserved with array_reserve or array_reserve_from out of loop/
/variable \$y3 can be reserved with array_reserve or array_reserve_from out of loop/
/variable \$y4 can be reserved with array_reserve or array_reserve_from out of loop/
/variable \$y5 can be reserved with array_reserve or array_reserve_from out of loop/
<?php

/**
 * @kphp-warn-performance array-reserve
 */
function test() {
  $y1 = [];
  for ($i = 0; $i != 1000; ++$i) {
    $y1[] = $i + 2;
  }

  $y2 = [];
  foreach ($y1 as $v) {
    $y2[] = $v;
  }

  $y3 = [];
  foreach ($y2 as $k => $v) {
    $y3[$k] = $v;
  }

  $y4 = [];
  $i = 1;
  while ($i < 1000) {
    $y4[] = ++$i;
  }

  $y5 = [];
  do {
    $y5[] = $i;
  } while ($i--);
}

test();
