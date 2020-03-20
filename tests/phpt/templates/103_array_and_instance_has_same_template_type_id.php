@kphp_should_fail
<?php

require_once 'polyfills.php';

/**
 * @kphp-template $obj1, $obj2
 */
function f($obj1, $obj2) {
    var_dump(count($obj1));
    var_dump(count($obj2));
}

f(new \Classes\A, [new \Classes\A]);

