@ok
<?php
require_once 'kphp_tester_include.php';

class A {
    public $x = 10;
}

/**
 * @kphp-infer
 * @return int
 */
function forkable_ten() {
    return 10;
}

/**
 * @kphp-infer
 * @return int
 */
function forkable_test() {
    $a = new A();
    $a->x += forkable_ten();
    return $a->x;
}

if (0) { fork(forkable_ten()); };

$id_test = fork(forkable_test());
var_dump(wait_result($id_test));
