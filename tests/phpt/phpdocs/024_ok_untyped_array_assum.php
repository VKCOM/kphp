@ok
<?php
require_once 'kphp_tester_include.php';

class Button {
    function f() {}
    function f2() {}
}

function getButtons() : array {
    $arr = [];
    if(1) { $arr[] = new Button; }
    return $arr;
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
