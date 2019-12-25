@ok
<?php
require_once 'polyfills.php';

class A {
    public $x = 10;
}

function forkable_ten() {
    return 10;
}

function forkable_test() {
    $a = new A();
    $a->x += forkable_ten();
    return $a->x;
}

if (0) { fork(forkable_ten()); };

$id_test = fork(forkable_test());
var_dump(wait_result($id_test));
