@kphp_should_fail
<?php
require_once 'polyfills.php';

function demo() {
    $t = true ? shape(['i' => 1, 's' => 'str']) : 2;
}

demo();
