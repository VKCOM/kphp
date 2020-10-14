@ok
<?php
require_once 'kphp_tester_include.php';
require_once 'kphp_tester_include.php';

/**
 * @param string $str
 * @param int $int
 * @return tuple(string, int[], \Classes\A)
 */
function constructT($str, $int) {
    return tuple($str, [1,2,$int], (new Classes\A)->setA($int));
}

/**
 * @return tuple(string, int[], \Classes\A)[]
 */
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
        // check that types are correctly inferred
        /** @var string */
        $str = $t[0];
        /** @var int */
        $int3 = $t[1][2];
        /** @var Classes\A */
        $a = $t[2];
        echo $str, ' ', $int3, ' ', $a->a, "\n";
    }
}

demo();
