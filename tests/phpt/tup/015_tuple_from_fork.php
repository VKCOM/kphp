@ok
<?php
require_once 'polyfills.php';
require_once 'Classes/autoload.php';

function demo() {
    sched_yield();
    return tuple(1, 'string');
}

$ii = fork(demo());
$t = wait_result($ii);
echo count($t), "\n";
