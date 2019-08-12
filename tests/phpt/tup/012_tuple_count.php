@ok
<?php
require_once 'polyfills.php';

function demo() {
    $t = tuple(1, 'str');
    echo count($t);         // works, but I don't known the real case to use this
}

demo();
