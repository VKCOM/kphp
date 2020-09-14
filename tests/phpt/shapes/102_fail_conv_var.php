@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

function demo() {
    $t = true ? shape(['i' => 1, 's' => 'str']) : 2;
}

demo();
