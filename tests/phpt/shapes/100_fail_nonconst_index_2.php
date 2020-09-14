@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

function demo() {
    $t = shape(['i' => 1, 's' => 'str']);
    $val = $t['i'.''];
}

demo();
