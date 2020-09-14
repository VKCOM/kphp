@ok
<?php
require_once 'kphp_tester_include.php';

/**
 * @kphp-infer
 * @param string $str
 * @param int $int
 * @return shape(a:\Classes\A, str:string, ints:int[])
 */
function constructT($str, $int) {
    return shape(['str' => $str, 'ints' => [1,2,$int], 'a' => (new Classes\A)->setA($int)]);
}

/**
 * @kphp-infer
 * @return shape(a:\Classes\A, str:string, ints:int[])[]
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
        $str = $t['str'];
        /** @var int */
        $int3 = $t['ints'][2];
        /** @var Classes\A */
        $a = $t['a'];
        echo $str, ' ', $int3, ' ', $a->a, "\n";
    }
}

demo();
