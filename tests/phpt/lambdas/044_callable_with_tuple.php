@ok
<?php

require_once 'polyfills.php';

class A {
    public $value = 10;
}

/**@param callable(int,tuple(A, shape(x:int), string)):int[] $get_ints*/
function foo($get_ints) {
    $ints = $get_ints(10, tuple(new A(), shape(['x' => 10]), "10"));
    var_dump($ints[0]);
}

foo(function($i, $t) { return [$i + $t[0]->value + $t[1]['x']]; });
foo(function($i, $t) { return [$i * $t[0]->value + $t[1]['x']]; });
