@ok
<?php

$x = 20;
/**
 * @param int $y
 */
function foo($y) {
    global $x;
    $x = 100;
    var_dump($y);
}

function run() {
    global $x;
    foo($x);
}

run();

