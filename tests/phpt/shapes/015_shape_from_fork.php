@ok
<?php
require_once 'polyfills.php';

/**
 * @kphp-infer
 * @return shape(i:int, s:string)
 */
function demo() {
  sched_yield();
  return shape(['i' => 1, 's' => 'string']);
}

$ii = fork(demo());
$t = wait_result($ii);
echo $t['i'], "\n";
echo $t['s'], "\n";
