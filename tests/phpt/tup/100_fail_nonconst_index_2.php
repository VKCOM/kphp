@kphp_should_fail
<?php
require_once 'polyfills.php';

function demo() {
    $t = tuple(1, 'str');
    $val = $t[1+0];
}

demo();
