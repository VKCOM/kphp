@ok
<?php
require_once 'kphp_tester_include.php';

function is_even($num) {
    return $num % 2 == 0;
}

function contains_a($str) {
    return strpos($str, "a") !== 0;
}

function test_array_all_empty() {
    $arr1 = [];
    var_dump(array_all($arr1, function ($v, $k) { return is_even($v); }));
}

function test_array_all_ints() {
    $arr1 = [1, 2, 3, 4, 5];
    var_dump(array_all($arr1, function ($v, $k) { return is_even($v); } ));

    $arr2 = [2, 4, 6, 8, 10];
    var_dump(array_all($arr2, function ($v, $k) { return is_even($v); } ));
}

function test_array_all_strings() {
    $arr1 = ["abcd", "bcd", "ab", "d"];
    var_dump(array_all($arr1, function ($v, $k) { return contains_a($v); }));

    $arr2 = ["abcd", "abc", "ab", "a"];
    var_dump(array_all($arr2, function ($v, $k) { return contains_a($v); }));
}

function test_array_all_single() {
    $arr1 = [1];
    var_dump(array_all($arr1, function ($v, $k) { return is_even($v); } ));

    $arr2 = [2];
    var_dump(array_all($arr2, function ($v, $k) { return is_even($v); } ));
}

function test_array_all_with_keys() {
    $arr1 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    var_dump(array_all($arr1, function ($v, $k) { return is_even($v); }));

    $arr2 = ["a" => 2, "b" => 4, "c" => 6, "d" => 8, "e" => 10];
    var_dump(array_all($arr2, function ($v, $k) { return is_even($v); }));

    var_dump(array_all($arr1, function ($v, $k) { return contains_a($k); }));

    $arr3 = ["a" => 1, "ab" => 2, "ac" => 3, "ad" => 4, "ae" => 5];
    var_dump(array_all($arr3, function ($v, $k) { return contains_a($k); }));
}

function test_array_all_empty_async_callback() {
    $arr1 = [];
    var_dump(array_all($arr1, function ($v, $k) {
        sleep(0.001);
        return is_even($v);
    }));
}

function test_array_all_ints_async_callback() {
    $arr1 = [1, 2, 3, 4, 5];
    var_dump(array_all($arr1, function ($v, $k) {
        sleep(0.001);
        return is_even($v);
    }));

    $arr2 = [2, 4, 6, 8, 10];
    var_dump(array_all($arr2, function ($v, $k) {
        sleep(0.001);
        return is_even($v);
    }));
}

function test_array_all_strings_async_callback() {
    $arr1 = ["abcd", "bcd", "ab", "d"];
    var_dump(array_all($arr1, function ($v, $k) {
        sleep(0.001);
        return contains_a($v);
    }));

    $arr2 = ["abcd", "abc", "ab", "a"];
    var_dump(array_all($arr2, function ($v, $k) {
        sleep(0.001);
        return contains_a($v);
    }));
}

function test_array_all_single_async_callback() {
    $arr1 = [1];
    var_dump(array_all($arr1, function ($v, $k) {
        sleep(0.001);
        return is_even($v);
    }));

    $arr2 = [2];
    var_dump(array_all($arr2, function ($v, $k) {
        sleep(0.001);
        return is_even($v);
    }));
}

function test_array_all_with_keys_async_callback() {
    $arr1 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    var_dump(array_all($arr1, function ($v, $k) {
        sleep(0.001);
        return is_even($v);
    }));

    $arr2 = ["a" => 2, "b" => 4, "c" => 6, "d" => 8, "e" => 10];
    var_dump(array_all($arr2, function ($v, $k) { 
        sleep(0.001);    
        return is_even($v);
    }));

    var_dump(array_all($arr1, function ($v, $k) {
        sleep(0.001);
        return contains_a($k);
    }));

    $arr3 = ["a" => 1, "ab" => 2, "ac" => 3, "ad" => 4, "ae" => 5];
    var_dump(array_all($arr3, function ($v, $k) {
        sleep(0.001);
        return contains_a($k);
    }));
}

test_array_all_empty();
test_array_all_ints();
test_array_all_strings();
test_array_all_single();
test_array_all_with_keys();
test_array_all_empty_async_callback();
test_array_all_ints_async_callback();
test_array_all_strings_async_callback();
test_array_all_single_async_callback();
test_array_all_with_keys_async_callback();
