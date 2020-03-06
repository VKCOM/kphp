@kphp_should_fail
/keys of shape\(\) must be constant/
<?php
require_once 'polyfills.php';

function demo() {
    $k = 'k';
    $t = shape(['a' => 1, $k => 'k']);
}

demo();
