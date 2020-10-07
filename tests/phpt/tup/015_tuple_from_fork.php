@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @kphp-infer
 * @return tuple(int, string)
 */
function demo() {
    sched_yield();
    return tuple(1, 'string');
}

$ii = fork(demo());
$t = wait($ii);
echo count($t), "\n";
