@ok php8
<?php
require_once 'kphp_tester_include.php';

use Classes\A;

function print_arr(array $a) {
    foreach($a as $elem) {
        echo ($elem);
    }
}

const A_CONST = new A(0);
const A_CONST2 = new A(42);
const arr = array(A_CONST, A_CONST2);

print_arr(arr);
print_arr(array(A_CONST2, A_CONST));
