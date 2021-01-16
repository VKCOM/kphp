@ok
<?php
require_once 'kphp_tester_include.php';

class Button {
    function f() {}
}

/** @return tuple(bool, Button[]) */
function getTuple() {
    return tuple(true, [new Button]);
}

function demo() {
    /** @var Button[] $b_arr */
    [$success, $b_arr] = 1 ? getTuple() : tuple(true, []);
    /** @var Button $b2 */
    $b2 = $b_arr[0];
    $b2->f();
}

demo();
