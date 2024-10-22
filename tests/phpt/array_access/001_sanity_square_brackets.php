@ok
<?php
require_once 'kphp_tester_include.php';

function test() {
    $arr = ["zero", "one", "two", 4 => "four", "str_key" => 42];
    $obj = new Classes\LikeArray();

    foreach ($arr as $k => $v) {
        $arr[$k] = $v;
    }

    foreach ($arr as $k => $v) {
        var_dump("Expect: $v\n");
        $v2 = $obj[$k];
        var_dump("Got: $v2\n");
    }
}

test();
