@kphp_should_fail
<?php
require_once 'polyfills.php';

function demo() {
    $t = tuple(1, 'str');
    if(1)
        $t = array('some other');
    echo count($t);
}

demo();
