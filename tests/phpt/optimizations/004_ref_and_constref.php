@ok
<?php

function bar($x, &$xs) {
    $xs[0] = 10;
    var_dump($x);
}

function foo(&$x, &$xs) {
    bar($x, $xs);
}

function run() {
    $xs = [1, "a"];
    foo($xs[0], $xs);
}

run();
