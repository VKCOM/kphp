@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

function demo() {
    $t = tuple(1, 'str');
    $val = $t[1+0];
}

demo();
