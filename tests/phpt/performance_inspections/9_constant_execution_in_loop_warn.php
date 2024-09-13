@kphp_should_warn k2_skip
/string building 'hello '.\$x.\$z\[floor\(sin\(\$x\)\)\] can be saved in a separate variable out of loop/
/array element \$z\[floor\(sin\(\$x\)\)\] can be saved in a separate variable out of loop/
/function call floor\(sin\(\$y2\[\$x\]\)\) can be saved in a separate variable out of loop/
/array element \$z\$v1\['abc'\]\[\$y1\[0\]\] can be saved in a separate variable out of loop/
<?php

/**
 * @kphp-warn-performance constant-execution-in-loop
 */
function test() {
  $x = 5.2;
  $z = ["hello" => "world"];

  $y1 = [];
  for ($i = 0; $i != 1000; ++$i) {
    $y1[] = "hello $x" . $z[floor(sin($x))];
  }

  $y2 = [];
  foreach ($y1 as $v) {
    $y2[] = $z[floor(sin($x))].$v;
  }

  $y3 = [];
  $i = 1;
  while ($i < 1000) {
    $y3[] = floor(sin($y2[(int)$x]));
  }

  $z = ["xxx" => $z];
  $y4 = 0;
  do {
    $y4 += (float)$z["abc"][$y1[0]];
  } while ($i--);
}

test();
