@kphp_should_fail
<?php
require_once 'polyfills.php';

function demo() {
    $t = tuple(1, 'str');
    $key = 1;
    $val = $t[$key];
}

demo();
