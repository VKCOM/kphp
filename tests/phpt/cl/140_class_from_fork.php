@ok
<?php
#ifndef KittenPHP
require_once 'polyfills.php';
require_once 'polyfills.php';
#endif

class Temp {
    public $x;
}

function demo() {
    sched_yield();
    $t = new Temp();
    $t->x = tuple(123, "asdf", new \Classes\A());

    return $t;
}

$ii = fork(demo());
$t = wait_result($ii);
echo "done\n";

var_dump(instance_to_array($t));
