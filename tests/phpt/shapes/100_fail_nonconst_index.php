@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

function demo() {
    $t = shape(['i' => 1, 's' => 'str']);
    $key = 1;
    $val = $t[$key];
}

demo();
