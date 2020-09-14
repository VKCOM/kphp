@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

function getMe($arg) {
    return $arg
        ? shape(['i' => 1, 's' => 'str'])
        : $arg;
}

function demo() {
    $t = getMe(true);
}

demo();
