@ok
<?php

require_once "Classes/autoload.php";

/**
 * @kphp-template $obj2
 */
function f($obj1, $obj2) {
    $obj2->magic = 777;
    $obj2->print_magic();
    var_dump($obj1);
}

// this f will have signature as f_instance_Classes$a(int, \Classes\B)
f(10, new \Classes\B);

// this f will have signature as f_instance_Classes$a(string, \Classes\A)
f("asdf", new \Classes\A);
