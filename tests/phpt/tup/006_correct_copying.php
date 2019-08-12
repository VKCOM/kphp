@ok
<?php
require_once 'polyfills.php';

function d() {
    $a = array(1,2,3);
    $t = tuple(1, 2, $a);
    $t2 = $t;

    $a[0] = 4;
    echo $a[0], "\n";
    echo $t[2][0], "\n";
    echo $t2[2][0], "\n";

    $a[2] = 99;
    echo $a[2], "\n";
    echo $t[2][2], "\n";
    echo $t2[2][2], "\n";
}

d();
