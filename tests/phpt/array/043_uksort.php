@ok
<?php

require_once 'kphp_tester_include.php';

function cmp_int($a, $b) {
    return $a <=> $b;
}

function cmp_str($a, $b) {
    return strcmp($a, $b);
}

function test_uksort_empty() {
    /** @var int[] */
    $arr1 = [];
    uksort($arr1, 'cmp_int');
    var_dump($arr1);
}

function test_uksort_ints() {
    $arr1 = [5, 1, 4, 2, 3];
    uksort($arr1, 'cmp_int');
    var_dump($arr1);

    $arr2 = [5, 1, 4, 2, 3, 2, 2, 5];
    uksort($arr2, 'cmp_int');
    var_dump($arr2);

    $arr3 = [1, 2, 3, 4, 5];
    uksort($arr3, 'cmp_int');
    var_dump($arr3);

    $arr4 = [5, 4, 3, 2, 1];
    uksort($arr4, 'cmp_int');
    var_dump($arr4);
}

function test_uksort_strings() {
    $arr1 = ["d", "b", "a", "e", "c"];
    uksort($arr1, 'cmp_int');
    var_dump($arr1);

    $arr2 = ["d", "b", "a", "e", "c", "e", "e", "d"];
    uksort($arr2, 'cmp_int');
    var_dump($arr2);

    $arr3 = ["a", "b", "c", "d", "e"];
    uksort($arr3, 'cmp_int');
    var_dump($arr3);

    $arr4 = ["e", "d", "c", "b", "a"];
    uksort($arr4, 'cmp_int');
    var_dump($arr4);    
}

function test_uksort_with_keys() {
    $arr1 = ["a" => 5, "b" => 1, "c" => 4, "d" => 2, "e" => 3];
    uksort($arr1, 'cmp_str');
    var_dump($arr1);

    $arr2 = ["a" => 5, "b" => 1, "c" => 4, "d" => 2, "e" => 3, "f" => 2, "g" => 2, "h" => 5];
    uksort($arr2, 'cmp_str');
    var_dump($arr2);

    $arr3 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    uksort($arr3, 'cmp_str');
    var_dump($arr3);

    $arr4 = ["a" => 5, "b" => 4, "c" => 3, "d" => 2, "e" => 1];
    uksort($arr4, 'cmp_str');
    var_dump($arr4);
}

function test_uksort_empty_async() {
    /** @var int[] */
    $arr1 = [];
    uksort($arr1, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr1);
}

function test_uksort_ints_async() {
    $arr1 = [5, 1, 4, 2, 3];
    uksort($arr1, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr1);

    $arr2 = [5, 1, 4, 2, 3, 2, 2, 5];
    uksort($arr2, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr2);

    $arr3 = [1, 2, 3, 4, 5];
    uksort($arr3, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr3);

    $arr4 = [5, 4, 3, 2, 1];
    uksort($arr4, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr4);
}

function test_uksort_strings_async() {
    $arr1 = ["d", "b", "a", "e", "c"];
    uksort($arr1, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr1);

    $arr2 = ["d", "b", "a", "e", "c", "e", "e", "d"];
    uksort($arr2, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr2);

    $arr3 = ["a", "b", "c", "d", "e"];
    uksort($arr3, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr3);

    $arr4 = ["e", "d", "c", "b", "a"];
    uksort($arr4, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr4);    
}

function test_uksort_with_keys_async() {
    $arr1 = ["d" => 1, "b" => 2, "a" => 3, "e" => 4, "c" => 5];
    uksort($arr1, function($a, $b) { sleep(0); return cmp_str($a, $b); });
    var_dump($arr1);

    $arr2 = ["d" => 1, "b" => 2, "a" => 3, "e" => 4, "c" => 5, "e" => 6, "e" => 7, "d" => 8];
    uksort($arr2, function($a, $b) { sleep(0); return cmp_str($a, $b); });
    var_dump($arr2);

    $arr3 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    uksort($arr3, function($a, $b) { sleep(0); return cmp_str($a, $b); });
    var_dump($arr3);

    $arr4 = ["e" => 1, "d" => 2, "c" => 3, "b" => 4, "a" => 5];
    uksort($arr4, function($a, $b) { sleep(0); return cmp_str($a, $b); });
    var_dump($arr4);    
}

test_uksort_empty();
test_uksort_ints();
test_uksort_strings();
test_uksort_with_keys();
test_uksort_empty_async();
test_uksort_ints_async();
test_uksort_strings_async();
test_uksort_with_keys_async();
