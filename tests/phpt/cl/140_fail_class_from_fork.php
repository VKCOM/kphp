@kphp_should_fail
<?php
#ifndef KittenPHP
require_once __DIR__.'/../dl/polyfill/fork-php-polyfill.php';
require_once 'Classes/autoload.php';
#endif

function demo() {
    sched_yield();
    return new Classes\A;
}

$ii = fork(demo());
$t = wait_result($ii);
echo "done\n";
