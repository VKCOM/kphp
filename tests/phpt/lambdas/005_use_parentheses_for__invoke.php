@ok no_php
<?php

require_once("Classes/autoload.php");

function get_lambda()
{
    return function ($x, $y) {
        return $x + $y;
    };
}

var_dump(get_lambda()(10, 20));

$f = function () {
    return 10;
};
var_dump($f());


$x = function () {
    return function ($x) {
        return function ($x) {
            return $x + 10;
        };
    };
};

var_dump($x()(10)(100));

$f2 = function() {
    $a = new Classes\CallbackHolder();
    return $a;
};

var_dump($f2()->get_callback()(10, 20) + 20);

