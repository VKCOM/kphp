@kphp_should_fail
/Accessing unexisting element of shape/
<?php
require_once 'kphp_tester_include.php';

function demo() {
    $t = shape(['i' => 1, 's' => 'str']);
    echo $t['a'];
}

demo();
