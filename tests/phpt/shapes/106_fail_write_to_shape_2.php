@kphp_should_fail
/shapes are read-only/
<?php
require_once 'polyfills.php';

function demo() {
    $a = [1,2,3,4,5];
    $t = shape(['i' => 1, 's' => 'str',  'a' => $a]);
    $t['a'][] = 66;
}

demo();
