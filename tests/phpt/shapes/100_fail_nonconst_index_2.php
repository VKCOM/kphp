@kphp_should_fail
<?php
require_once 'polyfills.php';

function demo() {
    $t = shape(['i' => 1, 's' => 'str']);
    $val = $t['i'.''];
}

demo();
