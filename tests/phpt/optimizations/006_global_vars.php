@ok
<?php

$x = 20;
/**
 * @kphp-infer
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

