@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @kphp-required
 * @param int $a
 * @param int $b
 * @return int
 */
function cmp(int $a, int $b) {
#ifndef KPHP
  sched_yield();
#endif
  return $b - $a;
}
function main() {
  $arr = [3, 2, 1];
  uasort($arr, 'cmp');
}
main();
