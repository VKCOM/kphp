@ok
<?php
require_once 'kphp_tester_include.php';

function demo1Instance() {
    $t = shape(['n' => 1, 's' => 'str', 'a' => new Classes\A]);

    $int = $t['n'];
    echo $int, "\n";

    /** @var Classes\A */
    $a = $t['a'];
    $a->printA();
}

/**
 * @return shape(a:\Classes\A, b:\Classes\B, n:int, s:string)
 */
function get2Instances() {
    return shape(['n' => 1, 's' => 'str', 'a' => new Classes\A, 'b' => new Classes\B]);
}

function demo2Instances() {
    $t = get2Instances();

    $int = $t['n'];
    echo $int, "\n";

    /** @var Classes\A */
    $a = $t['a'];
    $a->printA();

    /** @var Classes\B */
    $b = $t['b'];
    echo $b->setB1(5)->b1, "\n";
}

demo1Instance();
demo2Instances();
