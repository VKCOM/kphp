@ok
<?php

require_once "Classes/autoload.php";

/**
 * @kphp-template $obj1
 */
function f($obj1) {
    var_dump(count($obj1));
}

// this f will have signature as f_instance_Classes$a(int, \Classes\B)
f(new \Classes\A);

// this f will have signature as f_instance_Classes$a(string, \Classes\A)
f([new \Classes\A]);

