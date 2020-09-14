@ok
<?php

require_once 'kphp_tester_include.php';

/**
 * @kphp-template $obj
 */
function f($obj) {
    $obj->magic = 777;
    $obj->print_magic();
}

f(new \Classes\A);
f(new \Classes\B);
