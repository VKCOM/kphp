@ok
<?php

interface I { function f(); }

class A implements I { function f() { echo "A f\n"; } }
class B implements I { function f() { echo "B f\n"; } }

/**
 * @kphp-generic T = A
 * @param T $a
 */
function withDef1($a = null) {
    if ($a === null) echo "null\n";
    else $a->f();
}

withDef1();
withDef1/*<B>*/();
withDef1(null);
withDef1/*<A>*/(null);
withDef1(new A);
withDef1(new B);
withDef1/*<B>*/();
withDef1/*<B>*/(null);
withDef1/*<B>*/(new B);


/**
 * @kphp-generic T = mixed, T2 = int[]
 * @param T[] $arr
 * @param T2 $arr2
 */
function withDef2($arr, $arr2 = []) {
    echo "count ", count($arr), " ", count($arr2), "\n";
}

withDef2([]);
withDef2/*<mixed>*/([]);
withDef2/*<mixed, int[]>*/([]);
withDef2/*<mixed, string[]>*/([], []);
withDef2/*<mixed, string[]>*/([], ['1']);
withDef2([new A], [new A]);


/**
 * @kphp-generic T: \I = \I
 * @param ?T $i
 */
function withDef3($i) {
    if ($i === null) echo "null\n";
    else $i->f();
}

withDef3(null);
withDef3/*<A>*/(null);
withDef3(new A);
withDef3(new B);


/**
 * @kphp-generic T1, T2 = mixed
 * @param T1 $a
 * @param T2 $b
 */
function withDef4($a, $b = null) {
    if ($a === null) echo "null\n";
    else $a->f();
    var_dump($b);
}

withDef4(new A);
withDef4(new B);
withDef4/*<A>*/(null);
withDef4/*<A>*/(null, 3);
withDef4/*<A, ?int>*/(null);
withDef4/*<A, ?int>*/(new A);


class CC {
    /**
     * @kphp-generic T: self = self
     * @param T $cc
     */
    function withDef6($cc) {
        if ($cc === null) echo "cc null\n";
        else $cc->ccMethod();
    }

    function ccMethod() {
        echo "cc\n";
    }
}

class CC2 extends CC {
    function ccMethod() {
        echo "cc2\n";
    }
}

$cc = new CC;
$cc->withDef6(null);
$cc->withDef6(new CC);
$cc->withDef6(new CC2);
$cc->withDef6/*<CC2>*/(null);


/**
 * @kphp-generic T1, T2 = T1
 * @param T1 $t1
 * @param ?T2 $t2
 */
function withDef7($t1, $t2) {
    if ($t1 !== null) $t1->f();
    if ($t2 !== null) $t2->f();
}


$a7 = new A;
withDef7($a7, null);
withDef7/*<A>*/($a7, null);
withDef7/*<A, ?B>*/($a7, null);
withDef7/*<?B>*/(new B, null);
withDef7(new B, null);


/**
 * @kphp-generic T1, T2 = T1, T3 = T1|T2, T4 = T1|T2|T3
 * @param T1[] $arr1
 * @param T2[] $arr2
 * @param T3[] $arr3
 * @param T4[] $arr4
 * @return (T1|T2|T3|T4)[]
 */
function pushInOne($arr1, $arr2 = [], $arr3 = [], $arr4 = []) {
    $result = [];
    foreach ($arr1 as $k => $v)
        $result[] = $v;
    foreach ($arr2 as $k => $v)
        $result[] = $v;
    foreach ($arr3 as $k => $v)
        $result[] = $v;
    foreach ($arr4 as $k => $v)
        $result[] = $v;
    return $result;
}

/** @var int[] */
$ints = pushInOne([1,2,3]);
$ints = pushInOne([1,2,3], []);
$ints = pushInOne([1,2,3], [4,5,6]);
$ints = pushInOne([1,2,3], [4,5,6], [7,8,9], [10,11,12]);
var_dump($ints);

/** @var mixed[] */
$mixeds = pushInOne([1,2,3], ['1','2','3']);
$mixeds = pushInOne([1,2,3], ['1','2','3'], [true], [1]);
var_dump($mixeds);
$mixeds2 = pushInOne([1,2,3], ['1','2','3'], [true], [false]);
var_dump($mixeds2);
$mixeds3 = pushInOne([1,2,3], ['1','2','3'], [true], ['str']);
var_dump($mixeds3);

$as = pushInOne([new A]);
$as = pushInOne([new A], [new A]);
$as[1]->f();

/** @var I[] */
$is = pushInOne([new A]);
$is = pushInOne([new A], [new B]);
$is = pushInOne([new A], [new B], [], [new B, new B]);
$is[3]->f();
