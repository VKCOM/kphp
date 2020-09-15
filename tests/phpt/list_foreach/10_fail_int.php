@kphp_should_fail
/Can't make const to be lvalue/
<?php

/** @var int[][] $xs */
$xs = [[1], [5, -1], [1, 0], [7, 5]];

foreach ($xs as list(1, )) {
  var_dump($x);
}
