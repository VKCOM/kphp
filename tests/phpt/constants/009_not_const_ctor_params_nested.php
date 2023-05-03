@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

use Classes\B;

function getInt(int $x) {
    return $x * $x + 1;
}

const CONST_B = new B(new A(getInt(32))
echo (CONST_B);
