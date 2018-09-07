@kphp_should_fail
<?php
require_once 'polyfill/tuple-php-polyfill.php';

function demo() {
    $t = tuple(1, 'str');
    $a = array_map(function($v) { return $v; }, $t);
}

demo();
