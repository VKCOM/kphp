@kphp_should_fail
<?php
require_once 'kphp_tester_include.php';

/**
 * @return tuple(int, string)|false
 */
function getT() {
    return true ? false : tuple(1, 'str');
}

function demo() {
    $t = getT();
    echo $t ? $t[-1] : 'nothing';
}

demo();
