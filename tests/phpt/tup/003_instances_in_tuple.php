@ok
<?php
require_once 'polyfills.php';
require_once 'polyfills.php';

function demo1Instance() {
    $t = tuple(1, 'str', new Classes\A);

    $int = $t[0];
    echo $int, "\n";

    /** @var Classes\A */
    $a = $t[2];
    $a->printA();
}

function get2Instances() {
    return tuple(1, 'str', new Classes\A, new Classes\B);
}

function demo2Instances() {
    $t = get2Instances();

    $int = $t[0];
    echo $int, "\n";

    /** @var Classes\A */
    $a = $t[2];
    $a->printA();

    /** @var Classes\B */
    $b = $t[3];
    echo $b->setB1(5)->b1, "\n";
}

demo1Instance();
demo2Instances();
