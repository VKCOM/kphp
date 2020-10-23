@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

// this fails because kphp does not support function overloading per-arguments,
// and $arg is tuple and int at the same time in getT1()
// (but the code is ok, it should be made to work in future)

function getT1($arg) {
    return tuple($arg, [$arg]);
}

function getT2() {
    return getT1(getT1(1));
}

function demo() {
    $t = getT2();
}

demo();
