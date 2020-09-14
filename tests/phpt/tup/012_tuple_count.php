@ok
<?php
require_once 'kphp_tester_include.php';

function demo() {
    $t = tuple(1, 'str');
    echo count($t);         // works, but I don't known the real case to use this
}

demo();
