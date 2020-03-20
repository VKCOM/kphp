@ok
<?php

require_once 'polyfills.php';

/**
 * @kphp-template $obj
 */
function f($obj) {
    $obj->magic = 777;
    $obj->print_magic();
}

f(new \Classes\A);
f(new \Classes\B);
