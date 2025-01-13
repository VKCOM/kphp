@ok
<?php
require_once 'kphp_tester_include.php';

class A {};

/** @return A */
function getObj(int $x) {
    if ($x == 0) {
        return null;
    }
    return new A();
}

/** @param $x mixed */
function is_obj_to_str($x) {
    if (is_object($x)) {
        return "true";
    }
    return "false";
}

function test() {
    /** @var mixed */
    $x;
    $a = new A();
    
    $x = to_mixed($a);
    echo is_obj_to_str($x) . "\n";

    $x = 123;
    echo is_obj_to_str($x) . "\n";

    #ifndef KPHP
    echo "false\n"; // PHP emits fatal error
    return;
    #endif
    $a = getObj(0);
    $x = to_mixed($a);
    echo is_obj_to_str($x) . "\n";
}

test();
