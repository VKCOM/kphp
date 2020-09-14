@ok
<?php
require_once 'kphp_tester_include.php';

define('IDX_0', 'one');
define('IDX_1', 's');

function demo() {
    $t = shape(['one' => 1, 's' =>'str']);
    echo $t[IDX_0], ' ';
    echo $t[IDX_1], ' ';
}
