@kphp_should_fail
/shapes are read-only/
<?php
require_once 'kphp_tester_include.php';

function demo() {
    $t = shape(['i' => 1, 's' => 'str']);
    $t[0] = 2;
}

demo();
