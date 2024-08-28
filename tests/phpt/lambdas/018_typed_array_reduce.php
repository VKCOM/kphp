@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

function test_primitive_types() {
    $f = function(int $c, int $item) {
        return $c + $item; 
    };

    $f(10, 20);

    $res = array_reduce([1, 2, 3], $f, 0);
    var_dump($res);
}

function test_classes() {
    $f = function (int $carry, Classes\IntHolder $item) {
        return $carry + $item->a;
    };

    $res = array_reduce([new Classes\IntHolder(9), new Classes\IntHolder(20)], $f, 1);
    var_dump($res);
}

function test_reduce_with_diff_return_and_carry() {
    $f = function (?Classes\IntHolder $carry, Classes\IntHolder $item) {
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
