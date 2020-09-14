@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

function demo() {
    $t = true ? tuple(1, 'str') : 2;
}

demo();
