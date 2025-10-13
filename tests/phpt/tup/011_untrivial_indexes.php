@ok
<?php
require_once 'kphp_tester_include.php';

define('IDX_0', 0);
define('IDX_1', 1);

function demo() {
    $t = tuple(1, 'str');
    echo $t[IDX_0], ' ';
    echo $t[IDX_1], ' ';
    echo $t[0x01], ' ';
    echo $t[0001], ' ';
    echo $t[+1], ' ';
}
