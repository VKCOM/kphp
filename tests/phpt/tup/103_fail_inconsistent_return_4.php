@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

// this fails until we support null as well as false

function getMe($arg) {
    return $arg
        ? tuple(1, 'str')
        : 1;
}

function demo() {
    $t = getMe(true);
}

demo();
