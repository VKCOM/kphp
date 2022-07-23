@ok
<?php

class A {
    function fa() { echo "A fa\n"; }
}

/**
 * @kphp-generic T
 * @param T $obj
 * @return T
 */
function tplCallFa($obj) {
    $obj->fa();
    return $obj;
}

($a = new A)->fa();

function demo1() {
    $b = ($a = new A);
    $b->fa();
}

function demo2() {
    $b1 = tplCallFa(new A);
    $b1->fa();
    $b2 = tplCallFa($a = new A);
    $b2->fa();
    tplCallFa($b2)->fa();
    tplCallFa(tplCallFa(new A))->fa();
}

function demo3() {
    $a1 = new A;
    $a2 = clone $a1;
    $a2->fa();

    (clone $a1)->fa();
    (clone new A)->fa();
}

demo1();
demo2();
demo3();

// -------------

interface I {
    function fa();
}

class Paren implements I {
    function fa() { echo "Paren fa\n"; }
}
class Child extends Paren {
    function fa() { echo "Child fa\n"; }
}

function callFa1(?Paren $o = null) {
    if (!$o)
        $o = new Child;
    $o->fa();
}

function callFa2() {
    $o = new Paren;
    $o->fa();
    $o = new Child;
    $o->fa();
}

function callFa3(?I $i) {
    if (!$i)
        $i = new Paren;
    $i->fa();
    $i = new Child;
    $i->fa();
}

callFa1(new Paren);
callFa1(null);

callFa2();

callFa3(new Child);
callFa3(null);

