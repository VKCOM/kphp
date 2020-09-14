@ok
<?php

require_once 'kphp_tester_include.php';

function test_static() {
    $f = function ($x) {
        static $i = 0;
        $i++;
        var_dump($x + $i);
    };

    $f(20000);
    $f(20000);
    $f(20000);
    $f(20000);
    $f(20000);
}

function test_static_array() {
    $f = function ($x) {
        static $h = [1, 2, 3];
        $h[0]++;
        
        var_dump($x + $h[0]);
    };

    $f(100000);
    $f(100000);
    $f(100000);
    $f(100000);
    $f(100000);
}

test_static();
test_static_array();
