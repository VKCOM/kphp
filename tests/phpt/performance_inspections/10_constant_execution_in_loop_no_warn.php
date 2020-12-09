@ok
KPHP_ERROR_ON_WARNINGS=1
<?php

/**
 * @kphp-warn-performance constant-execution-in-loop
 */
function test() {
  $x = 5.2;
  $z = ["hello" => "world"];

  $y1 = [];
  for ($i = 0; $i != 10; ++$i) {
    $y1[] = "hello $i" . $z[floor(sin($x + $i))];
  }

  $y2 = [];
  foreach ($y1 as $v) {
    if (1) {
      $y2[] = $z[floor(sin($x))] . $v;
    }
  }

  $y3 = [];
  $i = 1;
  while ($i < 10) {
    $y3[] = (int)$y2[1] + $i++;
  }

  $z = ["xxx" => $z];
  $y4 = 0;
  do {
    if ($y4 < 2 && (float)$z["abc"][(int)$y1[0]] > 2) {
      $y4 += 0.4 * $i - (float)$z["abc"][(int)$y1[0]];
    }
  } while ($i--);
}

test();
