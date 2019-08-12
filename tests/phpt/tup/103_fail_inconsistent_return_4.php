@kphp_should_fail
<?php
require_once 'polyfills.php';

// this fails until we support null as well as false

function getMe($arg) {
    return $arg
        ? tuple(1, 'str')
        : null;
}

function demo() {
    $t = getMe(true);
}

demo();
