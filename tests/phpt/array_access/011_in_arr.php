@ok
<?php
require_once 'kphp_tester_include.php';

function test() {
    $o = new Classes\LoggingLikeArray([null, 1 => [1 => 42]]);
    $a = [[$o]];

    var_dump($o[0][0][0]);
    var_dump($o[0][0][1][1]);

    var_dump(isset($a[0][0][1][1]));
    var_dump(isset($a[0][0][0]));

    var_dump(empty($a[0][0][1][1]));
    var_dump(empty($a[0][0][0]));

    unset($a[0][0][1]);
    unset($a[0][0][0]);

    var_dump(isset($a[0][0][1][1]));
    var_dump(isset($a[0][0][0]));

    var_dump(empty($a[0][0][1][1]));
    var_dump(empty($a[0][0][0]));
}

test();
