@kphp_should_fail
<?php
require_once 'polyfills.php';

function demo() {
    $t = true ? tuple(1, 'str') : 2;
}

demo();
