@ok
<?php

class A {
    public $x = 10;
}

/**
 * @kphp-const $a
 * @param A $a
 */
function foo($a) {
    $a->x = 20;
}


foo(new A());
