@ok
<?php

require_once("polyfills.php");

function test_primitive_capturing($y) {
    $f = function ($x) use ($y) {
        return $x + $y;
    };

    var_dump($f(100));
}

function test_returning_lambda_with_captured_values($x) {
    return function ($y) use ($x) {
        return $y + $x;
    };
}

function test_capturing_classes() {
    $x = new Classes\IntHolder(10);
    $f = function ($y) use ($x) {
        return $y + $x->a;
    };

    var_dump($f(100));
}

$x = 200;
test_primitive_capturing($x);
test_capturing_classes();
$callback = test_returning_lambda_with_captured_values(100);
var_dump($callback(20));

// function test_capturing_already_captured_variables() {
//     $x = 10;
//     $f = function () use ($x) {
//         $f2 = function () use ($x) {
//             return $x;
//         };
// 
//         return $f2();
//     };
// 
//     var_dump($f());
// }
// test_capturing_already_captured_variables();

