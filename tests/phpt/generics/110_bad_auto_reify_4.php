@kphp_should_fail
/Couldn't reify generic <T> for call/
<?php

/**
 * @kphp-generic T
 * @param T $arr
 */
function f($arr) {
    var_dump($arr);
}

f([1,2,'3']);

