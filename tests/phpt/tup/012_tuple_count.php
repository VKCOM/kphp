@ok
<?php
require_once 'polyfill/tuple-php-polyfill.php';

function demo() {
    $t = tuple(1, 'str');
    echo count($t);         // works, but I don't known the real case to use this
}

demo();
