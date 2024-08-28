@ok k2_skip
<?php
#ifndef KPHP
require_once 'kphp_tester_include.php';
require_once 'kphp_tester_include.php';
#endif

class Temp {
    /** @var tuple(int, string, \Classes\A) */
    public $x;
}

/**
 * @return Temp
 */
function demo() {
    sched_yield();
    $t = new Temp();
    $t->x = tuple(123, "asdf", new \Classes\A());

    return $t;
}

$ii = fork(demo());
$t = wait($ii);
echo "done\n";

var_dump(to_array_debug($t));
