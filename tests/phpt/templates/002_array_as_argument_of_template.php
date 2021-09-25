@ok
<?php

require_once 'kphp_tester_include.php';

class A {
    function fa() { echo "fa\n"; }
}

class B {
    function fa() { echo "fb\n"; }
}

/**
 * @kphp-template $obj
 */
function f($obj) {
    foreach ($obj as $o) {
        $o->magic = 777;
        $o->print_magic();
    }
}

f([new \Classes\A, new \Classes\A]);
f([new \Classes\B, new \Classes\B]);


/** @kphp-template $arr */
function acceptArrayTurnsOptional($arr) {
    if ($arr) $arr[0]->fa();
    else echo "arr empty or null\n";
}
/** @var ?A[] */
$arr_opt_A = [new A];
if (1) $arr_opt_A = null;
acceptArrayTurnsOptional($arr_opt_A);
acceptArrayTurnsOptional([new A]);
acceptArrayTurnsOptional([new B]);

