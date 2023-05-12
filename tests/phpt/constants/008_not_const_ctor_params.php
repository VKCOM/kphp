@ok kphp_should_fail
/Non-const expression in const context/
<?php
require_once 'kphp_tester_include.php';

use Classes\A;

function getInt(int $x) {
    return $x * $x + 1;
}

const CONST_A = new A(getInt(42));
echo(CONST_A);
