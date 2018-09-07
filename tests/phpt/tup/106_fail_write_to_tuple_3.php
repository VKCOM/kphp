@kphp_should_fail
<?php
require_once 'polyfill/tuple-php-polyfill.php';

function demo() {
    $t = tuple(1, 'str');
    $t[] = 'new';
    echo $t[1];
}

demo();
