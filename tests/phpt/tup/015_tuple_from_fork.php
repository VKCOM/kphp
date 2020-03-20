@ok
<?php
require_once 'polyfills.php';

function demo() {
    sched_yield();
    return tuple(1, 'string');
}

$ii = fork(demo());
$t = wait_result($ii);
echo count($t), "\n";
