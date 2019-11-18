@ok
<?php

$x = 20;
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

