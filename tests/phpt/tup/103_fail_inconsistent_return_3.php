@kphp_should_fail
<?php
require_once 'polyfills.php';

function getMe($arg) {
    return $arg
        ? tuple(1, 'str')
        : $arg;
}

function demo() {
    $t = getMe(true);
}

demo();
