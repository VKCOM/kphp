@ok
<?php
require_once 'kphp_tester_include.php';

function test() {
    $arr = ["zero", "one", "two", 4 => "four", "str_key" => 42];
    $obj = new Classes\LoggingLikeArray();

    foreach ($arr as $k => $v) {
        $obj[$k] = $v;
    }

    foreach ($arr as $k => $v) {
        $v2 = $obj[$k];
        assert_true($v == $v2);
    }
}

test();
