@ok
<?php

require_once 'kphp_tester_include.php';

function cmp_int(int $a, int $b) {
    return $a <=> $b;
}

function cmp_str(string $a, string $b) {
    return strcmp($a, $b);
}

function test_usort_empty() {
    /** @var int[] */
    $arr1 = [];
    usort($arr1, 'cmp_int');
    var_dump($arr1);
}

function test_usort_ints() {
    $arr1 = [5, 1, 4, 2, 3];
    usort($arr1, 'cmp_int');
    var_dump($arr1);

    $arr2 = [5, 1, 4, 2, 3, 2, 2, 5];
    usort($arr2, 'cmp_int');
    var_dump($arr2);

    $arr3 = [1, 2, 3, 4, 5];
    usort($arr3, 'cmp_int');
    var_dump($arr3);

    $arr4 = [5, 4, 3, 2, 1];
    usort($arr4, 'cmp_int');
    var_dump($arr4);
}

function test_usort_strings() {
    $arr1 = ["d", "b", "a", "e", "c"];
    usort($arr1, 'cmp_str');
    var_dump($arr1);

    $arr2 = ["d", "b", "a", "e", "c", "e", "e", "d"];
    usort($arr2, 'cmp_str');
    var_dump($arr2);

    $arr3 = ["a", "b", "c", "d", "e"];
    usort($arr3, 'cmp_str');
    var_dump($arr3);

    $arr4 = ["e", "d", "c", "b", "a"];
    usort($arr4, 'cmp_str');
    var_dump($arr4);    
}

function test_usort_with_keys() {
    $arr1 = ["a" => 5, "b" => 1, "c" => 4, "d" => 2, "e" => 3];
    usort($arr1, 'cmp_int');
    var_dump($arr1);

    $arr2 = ["a" => 5, "b" => 1, "c" => 4, "d" => 2, "e" => 3, "f" => 2, "g" => 2, "h" => 5];
    usort($arr2, 'cmp_int');
    var_dump($arr2);

    $arr3 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    usort($arr3, 'cmp_int');
    var_dump($arr3);

    $arr4 = ["a" => 5, "b" => 4, "c" => 3, "d" => 2, "e" => 1];
    usort($arr4, 'cmp_int');
    var_dump($arr4);
}

function test_usort_empty_async() {
    /** @var int[] */
    $arr1 = [];
    usort($arr1, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr1);
}

function test_usort_ints_async() {
    $arr1 = [5, 1, 4, 2, 3];
    usort($arr1, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr1);

    $arr2 = [5, 1, 4, 2, 3, 2, 2, 5];
    usort($arr2, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr2);

    $arr3 = [1, 2, 3, 4, 5];
    usort($arr3, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr3);

    $arr4 = [5, 4, 3, 2, 1];
    usort($arr4, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr4);
}

function test_usort_strings_async() {
    $arr1 = ["d", "b", "a", "e", "c"];
    usort($arr1, function($a, $b) { sleep(0); return cmp_str($a, $b); });
    var_dump($arr1);

    $arr2 = ["d", "b", "a", "e", "c", "e", "e", "d"];
    usort($arr2, function($a, $b) { sleep(0); return cmp_str($a, $b); });
    var_dump($arr2);

    $arr3 = ["a", "b", "c", "d", "e"];
    usort($arr3, function($a, $b) { sleep(0); return cmp_str($a, $b); });
    var_dump($arr3);

    $arr4 = ["e", "d", "c", "b", "a"];
    usort($arr4, function($a, $b) { sleep(0); return cmp_str($a, $b); });
    var_dump($arr4);    
}

function test_usort_with_keys_async() {
    $arr1 = ["a" => 5, "b" => 1, "c" => 4, "d" => 2, "e" => 3];
    usort($arr1, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr1);

    $arr2 = ["a" => 5, "b" => 1, "c" => 4, "d" => 2, "e" => 3, "f" => 2, "g" => 2, "h" => 5];
    usort($arr2, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr2);

    $arr3 = ["a" => 1, "b" => 2, "c" => 3, "d" => 4, "e" => 5];
    usort($arr3, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr3);

    $arr4 = ["a" => 5, "b" => 4, "c" => 3, "d" => 2, "e" => 1];
    usort($arr4, function($a, $b) { sleep(0); return cmp_int($a, $b); });
    var_dump($arr4);
}

test_usort_empty();
test_usort_ints();
test_usort_strings();
test_usort_with_keys();
test_usort_empty_async();
test_usort_ints_async();
test_usort_strings_async();
test_usort_with_keys_async();
