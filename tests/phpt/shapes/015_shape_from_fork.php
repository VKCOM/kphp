@ok
<?php
require_once 'polyfills.php';
require_once 'Classes/autoload.php';

function demo() {
  sched_yield();
  return shape(['i' => 1, 's' => 'string']);
}

$ii = fork(demo());
$t = wait_result($ii);
echo $t['i'], "\n";
echo $t['s'], "\n";
