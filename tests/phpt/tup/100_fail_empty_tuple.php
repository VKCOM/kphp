@kphp_should_fail
<?php
require_once 'polyfill/tuple-php-polyfill.php';

function demo() {
    $t = tuple();
}

demo();
