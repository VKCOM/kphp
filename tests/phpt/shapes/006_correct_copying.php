@ok
<?php
require_once 'kphp_tester_include.php';

function d() {
    $a = array(1,2,3);
    $t = shape(['n1' => 1, 'n2' => 2, 'arr' => $a]);
    $t2 = $t;

    $a[0] = 4;
    echo $a[0], "\n";
    echo $t['arr'][0], "\n";
    echo $t2['arr'][0], "\n";

    $a[2] = 99;
    echo $a[2], "\n";
    echo $t['arr'][2], "\n";
    echo $t2['arr'][2], "\n";
}

d();
