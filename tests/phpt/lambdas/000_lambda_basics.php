@ok
<?php

require_once 'kphp_tester_include.php';

class A {
    function fa() { echo "fa\n"; }
}

$fun1 = function () {
    return 10;
};
var_dump($fun1());


$fun2 = function ($x, $y) {
    return $x + $y;
};
var_dump($fun2(10, 20));


/**
 * @kphp-template $callback
 */
function takeAndCall($callback)
{
    var_dump($callback(10, 20));
}

$f1 = function ($x, $y) {
    return $x + $y;
};
takeAndCall($f1);
takeAndCall(function ($x, $y) {
    return $x * $y;
});


function callOfInstance(callable $callback)
{
    $a = new Classes\IntHolder();
    $res = $callback($a);
    var_dump($res);
}

$f11 = function (Classes\IntHolder $a) {
    return $a->a + 10;
};
callOfInstance($f11);
callOfInstance(function (Classes\IntHolder $a) {
    return $a->a + 10000;
});


$f_with_defaults = function($x = 10) {
    return $x + 10;
};

var_dump($f_with_defaults());

(function() { echo "inline function expression all\n"; })();
(function($arg) { echo "inline function expression call with $arg\n"; })(100);


function callable_lower_f(callable $f) {
    return $f(10);
}

/**
 * @param callable $f
 * @return int
 */
function kphp_callable_f($f) {
    return $f(10);
}

$callable = function ($x) { return $x + 1; };
var_dump(callable_lower_f($callable));
var_dump(kphp_callable_f($callable));


function demoWithAssumption($x = 'x') {
    $f = function() use($x) :A { echo $x; return new A; };
    $f();
    $f()->fa();
}
demoWithAssumption();

function takesCallableReturnA(callable $cb): A {
    echo $cb(), "\n";
    return new A;
}
takesCallableReturnA(fn() => 1)->fa();
takesCallableReturnA(fn() => 'str')->fa();

