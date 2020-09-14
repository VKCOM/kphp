@kphp_should_fail
/shape\(\) must have an array associative non-empty inside/
<?php
require_once 'kphp_tester_include.php';

function demo() {
    $t = shape(123);
}

demo();
