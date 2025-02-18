@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @return shape(i:int, s:string)
 */
function demo() {
  sched_yield();
  return shape(['i' => 1, 's' => 'string']);
}

$ii = fork(demo());
$t = wait($ii);
echo $t['i'], "\n";
echo $t['s'], "\n";
