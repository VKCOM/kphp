@kphp_should_fail
<?php

require_once 'kphp_tester_include.php';

/**
 * @kphp-template $obj1, $obj2 comment here
 * @param int $obj1 
 */
function f($obj1, $obj2) {
    $obj1->magic = 777;
    $obj1->print_magic();
}

f(new \Classes\A, new \Classes\B);
