@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

function demo() {
    $a = [1,2,3,4,5];
    $t = tuple(1, 'str', $a);
    $t[2][] = 66;
}

demo();
