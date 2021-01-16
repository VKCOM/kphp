@kphp_should_fail
/demo\(\)::\$b is both primitive and Button/
/result of operation is both tuple\(primitive,Button\[\]\) and tuple\(primitive,primitive\)/
<?php

// todo: it's bad, that this test fails
// it's because of mixing any[] and class assumptions
// I hope to get it work somewhen

class Button {
    function f() {}
    function f2() {}
}

function getButtons() : array {
    $arr = [];
    if(1) { $arr[] = new Button; }
    return $att;
}

/** @return tuple(bool, Button[]) */
function getTuple() {
    return tuple(true, [new Button]);
}

function demo() {
    foreach (getButtons() as $b) {
        /** @var Button $b */
        $b->f();
    }
}

function demo2() {
    /** @var Button[] $b_arr */
    [$success, $b_arr] = 1 ? getTuple() : tuple(true, []);
    $b_arr[0]->f2();
}

demo();
demo2();
