<?php

require_once 'kphp_tester_include.php';

/**
 * @kphp-template T[] $arg $arg2
 * @kphp-return T
 */
function get_first($arg, $arg2, $get_first) {
    if ($get_first) {
        return $arg[0];
    } else {
        return $arg2[0];
    }
}

function test_return_element_of_template_array() {
    $arr = [new Classes\A(10), new Classes\A()];
    $arr2 = [new Classes\A(777)];

    $arr_0 = get_first($arr, $arr2, true);
    $arr2_0 = get_first($arr, $arr2, false);

    var_dump($arr_0->magic);
    var_dump($arr2_0->magic);
}

function test_return_another_element_of_template_array() {
    $arr = [new Classes\B(10), new Classes\B()];
    $arr2 = [new Classes\B(777)];

    $arr_0 = get_first($arr, $arr2, true);
    $arr2_0 = get_first($arr, $arr2, false);

    var_dump($arr_0->magic);
    var_dump($arr2_0->magic);
}

/**
 * @kphp-generic T $arg0 $arg1 $arg2
 * @kphp-return T[]
 */
function create_array($arg0, $arg1, $arg2) {
    return [$arg0, $arg1, $arg2];
}

function test_create_array_of_template_args() {
    $arg0 = new Classes\A(9);
    $arg1 = new Classes\A(8);
    $arg2 = new Classes\A(7);

    $arr = create_array($arg0, $arg1, $arg2);
    foreach ($arr as $a) {
        var_dump($a->magic);
    }
}

function test_create_another_array_of_template_args() {
    $arg0 = new Classes\B(9);
    $arg1 = new Classes\B(8);
    $arg2 = new Classes\B(7);

    $arr = create_array($arg0, $arg1, $arg2);
    foreach ($arr as $a) {
        var_dump($a->magic);
    }
}

test_return_element_of_template_array();
test_return_another_element_of_template_array();

test_create_array_of_template_args();
test_create_another_array_of_template_args();
