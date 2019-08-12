@ok
<?php
require_once 'polyfills.php';
require_once 'Classes/autoload.php';

function constructT($str, $int) {
    return tuple($str, [1,2,$int], (new Classes\A)->setA($int));
}

function getArrOfT() {
    return [
        constructT('s1', 1),
        constructT('s2', 2),
        constructT('s3', 3),
    ];
}

function demo() {
    $arr = getArrOfT();
    foreach ($arr as $t) {
        $str = $t[0];
        $int3 = $t[1][2];
        // check that types are correctly inferred
        $int3 /*:= int */;
        $str /*:= string */;
        /** @var Classes\A */
        $a = $t[2];
        echo $str, ' ', $int3, ' ', $a->a, "\n";
    }
}

demo();
