@ok
<?php
require_once 'kphp_tester_include.php';

function test() {
    $arr = ["zero", 0.125, "str_key" => 42];
    $obj = new Classes\LoggingLikeArray();

    foreach ($arr as $k => $v) {
        $obj[$k] = $v;
    }

    if ($obj instanceof Classes\LoggingLikeArray) {
        var_dump($obj[0]);
        var_dump($obj["str_key"]);
    } else {
        assert_true(false);
    }

    if ($obj instanceof Classes\LikeArray) {
        assert_true(false);
    }

    if ($obj instanceof \ArrayAccess) {
        var_dump($obj[1]);
        var_dump($obj[3]);
    } else {
        assert_true(false);
    }
}

test();
