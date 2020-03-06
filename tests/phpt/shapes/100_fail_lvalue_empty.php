@kphp_should_fail
/shapes are read-only/
<?php
require_once 'polyfills.php';

function demo() {
    $t = shape(['i' => 1, 's' => 'str']);
    $t[] = 'new';
    echo $t[1];
}

demo();
