@ok
<?php

require_once("Classes/autoload.php");

function test_primitive_types() {
    $f = function($c, $item) { 
        $c /*:= int*/;
        $item /*:= int*/;
        return $c + $item; 
    };

    $f(10, 20);

    $res = array_reduce([1, 2, 3], $f, 0);
    var_dump($res);
}

function test_classes() {
    $f = function ($carry, $item) {
        $carry /*:= int*/;
        return $carry + $item->a;
    };

    $res = array_reduce([new Classes\IntHolder(9), new Classes\IntHolder(20)], $f, 1);
    var_dump($res);
}

function test_reduce_with_diff_return_and_carry() {
    $f = function ($carry, $item) {
        if (!$carry) {
            return $item;
        }
        return new Classes\IntHolder($carry->a + $item->a);
    };

    /** @var Classes\IntHolder */
    $res = array_reduce([new Classes\IntHolder(9), new Classes\IntHolder(20)], $f, null);

    var_dump($res->a);
}

function test_reduce_array_of_arrays() {
    $f = function ($carry, $item) {
        if (!$carry) {
            return $item;
        }
        return array_merge($carry, $item);
    };

    /** @var Classes\IntHolder[]|false */
    $res = array_reduce([[new Classes\IntHolder(9)], [new Classes\IntHolder(20)]], $f, false);

    var_dump(count($res));
    var_dump($res[1]->a);
}

test_primitive_types();
test_classes();
test_reduce_with_diff_return_and_carry();
test_reduce_array_of_arrays();
