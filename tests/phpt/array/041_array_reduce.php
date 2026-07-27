@ok
<?php

require_once 'kphp_tester_include.php';

function sum($a, $b) {
    return $a + $b;
}

function concat($a, $b) {
    return $a + $b;
}

function test_array_reduce_empty() {
    /** @var int[] */
    $arr1 = [];
    var_dump(array_reduce($arr1, 'sum', 0));
}

function test_array_reduce_ints() {
    $arr1 = [1, 2, 3, 4, 5];
    var_dump(array_reduce($arr1, 'sum', 0));
}

function test_array_reduce_strings() {
    $arr1 = ["abcd", "bcd", "ab", "d"];
    var_dump(array_reduce($arr1, 'concat', ""));
}

function test_array_reduce_with_keys() {
    $arr1 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    var_dump(array_reduce($arr1, 'sum', 0));
}

function test_array_reduce_empty_async() {
    /** @var int[] */
    $arr1 = [];
    var_dump(array_reduce($arr1, function($a, $b) { sleep(0); return sum($a, $b); } , 0));
}

function test_array_reduce_ints_async() {
    $arr1 = [1, 2, 3, 4, 5];
    var_dump(array_reduce($arr1, function($a, $b) { sleep(0); return sum($a, $b); }, 0));
}

function test_array_reduce_strings_async() {
    $arr1 = ["abcd", "bcd", "ab", "d"];
    $init = "";
    var_dump(array_reduce($arr1, function($a, $b) { sleep(0); return concat($a, $b); }, $init));
}

function test_array_reduce_with_keys_async() {
    $arr1 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    var_dump(array_reduce($arr1, function($a, $b) { sleep(0); return sum($a, $b); }, 0));
}

test_array_reduce_empty();
test_array_reduce_ints();
test_array_reduce_strings();
test_array_reduce_with_keys();
test_array_reduce_empty_async();
test_array_reduce_ints_async();
test_array_reduce_strings_async();
test_array_reduce_with_keys_async();
