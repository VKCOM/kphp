@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

/**
 * @return tuple(int, string)
 */
function demo() {
    sched_yield();
    return tuple(1, 'string');
}

$ii = fork(demo());
$t = wait($ii);
echo count($t), "\n";
