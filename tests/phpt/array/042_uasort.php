@ok
<?php

require_once 'kphp_tester_include.php';

function cmp_int($a, $b) {
    return $a <=> $b;
}

function cmp_str($a, $b) {
    return strcmp($a, $b);
}

function test_uasort_empty() {
    /** @var int[] */
    $arr1 = [];
    uasort($arr1, 'cmp_int');
    var_dump($arr1);
}

function test_uasort_ints() {
    $arr1 = [5, 1, 4, 2, 3];
    uasort($arr1, 'cmp_int');
    var_dump($arr1);

    $arr2 = [1, 2, 3, 4, 5];
    uasort($arr2, 'cmp_int');
    var_dump($arr2);

    $arr3 = [5, 4, 3, 2, 1];
    uasort($arr3, 'cmp_int');
    var_dump($arr3);
}

function test_uasort_strings() {
    $arr1 = ["d", "b", "a", "e", "c"];
    uasort($arr1, 'cmp_str');
    var_dump($arr1);

    $arr2 = ["a", "b", "c", "d", "e"];
    uasort($arr2, 'cmp_str');
    var_dump($arr2);

    $arr3 = ["e", "d", "c", "b", "a"];
    uasort($arr3, 'cmp_str');
    var_dump($arr3);    
}

function test_uasort_with_keys() {
    $arr1 = ["a" => 5, "b" => 1, "c" => 4, "d" => 2, "e" => 3];
    uasort($arr1, 'cmp_int');
    var_dump($arr1);

    $arr2 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    uasort($arr2, 'cmp_int');
    var_dump($arr2);

    $arr3 = ["a" => 5, "b" => 4, "c" => 3, "d" => 2, "e" => 1];
    uasort($arr3, 'cmp_int');
    var_dump($arr3);
}

function test_uasort_empty_async() {
    /** @var int[] */
    $arr1 = [];
    uasort($arr1, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr1);
}

function test_uasort_ints_async() {
    $arr1 = [5, 1, 4, 2, 3];
    uasort($arr1, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr1);

    $arr2 = [1, 2, 3, 4, 5];
    uasort($arr2, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr2);

    $arr3 = [5, 4, 3, 2, 1];
    uasort($arr3, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr3);
}

function test_uasort_strings_async() {
    $arr1 = ["d", "b", "a", "e", "c"];
    uasort($arr1, function($a, $b) { sleep(0); return cmp_str($a, $b); });
    var_dump($arr1);

    $arr2 = ["a", "b", "c", "d", "e"];
    uasort($arr2, function($a, $b) { sleep(0); return cmp_str($a, $b); });
    var_dump($arr2);

    $arr3 = ["e", "d", "c", "b", "a"];
    uasort($arr3, function($a, $b) { sleep(0); return cmp_str($a, $b); });
    var_dump($arr3);    
}

function test_uasort_with_keys_async() {
    $arr1 = ["a" => 5, "b" => 1, "c" => 4, "d" => 2, "e" => 3];
    uasort($arr1, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr1);

    $arr2 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    uasort($arr2, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr2);

    $arr3 = ["a" => 5, "b" => 4, "c" => 3, "d" => 2, "e" => 1];
    uasort($arr3, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr3);
}

test_uasort_empty();
test_uasort_ints();
test_uasort_strings();
test_uasort_with_keys();
test_uasort_empty_async();
test_uasort_ints_async();
test_uasort_strings_async();
test_uasort_with_keys_async();
