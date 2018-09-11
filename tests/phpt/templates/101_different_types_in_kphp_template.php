@kphp_should_fail
<?php

require_once "Classes/autoload.php";

// @kphp-template guarantees that $obj1 and $obj2 will have the same type
/**
 * @kphp-template $obj1, $obj2 comment here
 */
function f($obj1, $obj2) {
    $obj1->magic = 777;
    $obj1->print_magic();
}

f(new \Classes\A, new \Classes\B);
