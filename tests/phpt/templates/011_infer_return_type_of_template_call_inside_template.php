@ok
<?php

require_once 'kphp_tester_include.php';

/**
 * @kphp-template $arg
 */
function HHHH($arg) {
    var_dump($arg->magic);
    return $arg;
}

/**
 * @kphp-template T $arg
 * @kphp-template $arg2
 * @kphp-return T
 */
function GGGG($arg, $arg2) {
    var_dump(count($arg2));
    return HHHH($arg);
}

function FFFF() {
    $b = new Classes\B;
    $b->magic = 100;
    $x = GGGG($b, [$b]);
    $y = GGGG($x, [1]);

    var_dump($y->magic);

    $a = new Classes\A;
    $a->magic = 200;
    GGGG($a, [$a]);
}


FFFF();


class AFiel {
    public $fiel = 0;
}

class BFiel {
    public $fiel = 1;
}

/**
 * @kphp-template T $obj
 * @kphp-return T
 */
function same($obj) {
    return $obj;
}

function demoNestedTplInfer() {
    $a = new AFiel;
    $a1 = same($a);
    $a2 = same($a1);
    $a3 = same($a2);
    $a1->fiel = 9;
    echo $a->fiel;
    echo $a1->fiel;
    echo $a2->fiel;
    echo $a3->fiel;
    echo "\n";

    $b = new BFiel;
    same(same(same($b)))->fiel = 10;
    echo same(same($b))->fiel;
    echo "\n";
}

demoNestedTplInfer();


class A {
    function fa() { echo "A fa\n"; }
}
class B {
    function fa() { echo "B fa\n"; }
}

/**
 * @kphp-template T
 * @kphp-param T $obj
 * @kphp-return T
 */
function tpl1($obj) {
    $obj->fa();
    return $obj;
}

/**
 * @kphp-template T
 * @kphp-param T $obj
 * @kphp-return T
 */
function tpl2($obj) {
    $obj->fa();
    return $obj;
}

tpl1(tpl2(tpl1(new A)))->fa();

$b = new B;
tpl2(tpl1(tpl2($b)))->fa();

