@ok
<?php

require_once 'kphp_tester_include.php';

function square($var) {
    return $var * $var;
}

function add_prefix($str) {
    return "prefix_" + $str;
}

function test_array_map_empty() {
    /** @var int[] */
    $arr1 = [];
    var_dump(array_map('square', $arr1));
}

function test_array_map_ints() {
    $arr1 = [1, 2, 3, 4, 5];
    var_dump(array_map('square', $arr1));
}

function test_array_map_strings() {
    $arr1 = ["abcd", "bcd", "ab", "d"];
    var_dump(array_map('add_prefix', $arr1));
}

function test_array_map_with_keys() {
    $arr1 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    var_dump(array_map('square', $arr1));
}

function test_array_map_empty_async() {
    /** @var int[] */
    $arr1 = [];
    var_dump(array_map(function($var) { sleep(0); return square($var); }, $arr1));
}

function test_array_map_ints_async() {
    $arr1 = [1, 2, 3, 4, 5];
    var_dump(array_map(function($var) { sleep(0); return square($var); }, $arr1));
}

function test_array_map_strings_async() {
    $arr1 = ["abcd", "bcd", "ab", "d"];
    var_dump(array_map(function($str) { sleep(0); return add_prefix($str); }, $arr1));
}

function test_array_map_with_keys_async() {
    $arr1 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    var_dump(array_map(function($var) { sleep(0); return square($var); }, $arr1));
}

test_array_map_empty();
test_array_map_ints();
test_array_map_strings();
test_array_map_with_keys();
test_array_map_empty_async();
test_array_map_ints_async();
test_array_map_strings_async();
test_array_map_with_keys_async();
