@kphp_should_fail
<?php
require_once 'polyfills.php';

function demo() {
    $t = tuple();
}

demo();
