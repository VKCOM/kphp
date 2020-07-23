@kphp_should_fail
/Accessing unexisting element of shape/
<?php
require_once 'polyfills.php';

/**
 * @kphp-infer
 * @return shape(i:int, s:string)|false
 */
function getT() {
    return true ? false : shape(['i' => 1, 's' => 'str']);
}

function demo() {
    $t = getT();
    echo $t ? $t[-1] : 'nothing';
}

demo();
