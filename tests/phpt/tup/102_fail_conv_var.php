@kphp_should_fail
<?php
require_once 'polyfill/tuple-php-polyfill.php';

function demo() {
    $t = true ? tuple(1, 'str') : 2;
}

demo();
