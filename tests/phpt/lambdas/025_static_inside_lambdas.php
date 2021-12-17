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

define('ab', 10);

$f = function() {
    static $a = [1, 2, 3, 4];
    $a[0] += 1;
    var_dump($a);
    var_dump(ab);
};

function XXX() {
    static $a = [1, 2, 3, 4];
    var_dump($a);
    var_dump(ab);
}

test_static();
test_static_array();

$f();
$f();
XXX();
