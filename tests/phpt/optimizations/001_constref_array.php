@ok
<?php

function dump($ar, $el) {
    $ar[0] = 10;
    var_dump($el);
}

function run() {
    $a = [1, "a"];
    dump($a, $a[0]);
}

run();
