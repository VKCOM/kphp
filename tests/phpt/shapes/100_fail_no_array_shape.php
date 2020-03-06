@kphp_should_fail
/shape\(\) must have an array associative non-empty inside/
<?php
require_once 'polyfills.php';

function demo() {
    $t = shape(123);
}

demo();
