@ok non-idempotent
<?php
#ifndef KPHP
?>
1
2
3
waited 0.03
waited 0.01
waited 0.02
<?php
exit;
#endif

/**
 * @param float $x
 * @param int $y
 * @return float
 */
function f($x, $y) {
  sched_yield_sleep($x);
  echo $y . "\n";
  return $x;
}

$ids = [];
$ids[] = fork(f(0.03, 3));
$ids[] = fork(f(0.01, 1));
$ids[] = fork(f(0.02, 2));

foreach ($ids as $id) {
  echo "waited ". wait($id) ."\n";
}
