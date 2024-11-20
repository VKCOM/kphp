@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

/**
 * @param $aa \ArrayAccess
 * @param $key mixed
 * @param $val mixed
 */
function print_and_change($aa, $key, $val) {
    var_dump($aa[$key]);
    $aa[$key] = $val;
}

function test() {
    $arr = ["zero", "one", "two", 4 => "four", "str_key" => 42];
    $obj = new Classes\LikeArray();

    foreach ($arr as $k => $v) {
        $obj[$k] = $v;
    }

    $keys = $obj->keys();
    foreach ($keys as $key) {
        print_and_change($obj, $key, "key_" . strval($key));
    }

    foreach ($keys as $key) {
        var_dump($obj[$key]);
    }
}

test();
