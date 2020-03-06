@kphp_should_fail
<?php
require_once 'polyfills.php';

function getMe($arg) {
    return $arg
        ? shape(['i' => 1, 's' => 'str'])
        : array(1, 'str');
}

function demo() {
    $t = getMe(true);
}

demo();
