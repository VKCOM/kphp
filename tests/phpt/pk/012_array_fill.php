@ok k2_skip
<?php

function demo1() {
    var_dump(array_fill(0, 10, 4));
}

function demo2() {
    var_dump(array_fill(5, 10, 4));
}

function demo3() {
    for ($i = 1; $i <= 100; $i++) {
        array_fill(0, $i, 0xcc);
    }
    $a = array_fill(0, 10, []);
    var_dump($a);
}

demo1();
demo2();
demo3();
