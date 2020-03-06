@kphp_should_fail
/Accessing unexisting element of shape/
<?php
require_once 'polyfills.php';

function demo() {
    $t = shape(['i' => 1, 's' => 'str']);
    echo $t['a'];
}

demo();
