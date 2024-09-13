@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class A {
    /** @var B */
    public $b;

    function f() { echo "A f\n"; }
}

class B {
    function f() { echo "B f\n"; }
}



/** @return A */
function getA1() { return new A; }
/** @return tuple(int, ?A) */
function getA2() { return tuple(1, new A); }

/** @return tuple(int, A[]|null) */
function getT2() {
    return tuple(1, [new A]);
}

/** @return tuple((A|null)[], string[]) */
function getT3() {
    return tuple([new A], []);
}

/** @return ?A[] */
function getArrOrNull() {
    return [new A];
}



function demo1() {
    $a = getA1();
    $a->f();
    $a = getA2()[1];
    $a->f();
}

function demo2() {
    if (1) {
        [, $arr] = getT2();
    } else {
        $arr = [];
    }

    foreach ($arr as $a) {
        $a->f();
    }
}

function demo3() {
    /** @var (A|null)[] $arr */
    [$arr] = getT3();
    $arr[0]->f();
}


function demo4() {
    $arr = [];
    $a = new A;
    $a->f();
    $arr[] = $a;

    foreach ($arr as $a) {
        $a->f();
    }
}

function demo5(?A $a) {
    $a->b = new B;
    if ($a->b)
        $a->b->f();
}

function demo6() {
    $arr = getArrOrNull();
    $a = 1 ? array_first_value($arr) : null;
    $a->f();
}




demo1();
demo2();
demo3();
demo4();
demo5(new A);
demo6();
