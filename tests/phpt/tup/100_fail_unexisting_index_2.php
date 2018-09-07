@kphp_should_fail
<?php
require_once 'polyfill/tuple-php-polyfill.php';

function demo() {
    $t = tuple(1, 'str');
    echo $t['strkey'];
}

demo();
