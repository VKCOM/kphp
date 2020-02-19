@ok
<?php

class A { public $x = 10; }

/** @return A | null */
function foo() {
    if (0) { return new A(); }
    return null;
}

function test() {
    if (foo() === null) {
        var_dump("adsf");
    } else {
        var_dump("123");
    }

    $x = foo();
    if ($x === null) {
        var_dump("adsf");
    } else {
        var_dump("123");
    }

    if (($x ?: foo()) === null) {
        var_dump("adsf");
    } else {
        var_dump("123");
    }
}

test();
