@ok k2_skip
<?php
require_once 'kphp_tester_include.php';


class A {
    function aMethod() {}
}

function getVoid1() {
    throw new Exception();
}
function getVoid2(): void {
    throw new Exception();
}

function getInt1(): int {
    throw new Exception();
}
function getInt2(): ?int {
    if (rand())
        throw new Exception();
    throw new Exception();
}

function getA1(): A {
    if (1) throw new Exception();
    else throw new Exception();
    return new A;
}
function getA2(): A {
    if (1) throw new Exception();
    else throw new Exception();
}

/** @return A[] */
function getArr() {
    switch (1) {
        case 1: exit();
        case 2: die();
        default: throw new Exception(1);
    }
}

function takeInt(int $arg) {}

try {
    getVoid1();
    getVoid2();
    takeInt(getInt1());
    takeInt(not_null(getInt2()));
    getA1()->aMethod();
    getA2()->aMethod();
    foreach (getArr() as $a)
        $a->aMethod();
}
catch (Exception $ex) {
    echo "ex caught, test ok, as it was compiled";
}
