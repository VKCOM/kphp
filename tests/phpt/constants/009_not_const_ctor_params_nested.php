@kphp_should_fail
/Non-const expression in const context/
<?php
require_once 'kphp_tester_include.php';

use Classes\B;
use Classes\A;

function getInt(int $x) {
    return $x * $x + 1;
}

const CONST_B = new B(new A(getInt(32)));
echo (CONST_B);
