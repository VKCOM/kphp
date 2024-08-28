@ok k2_skip
<?php

require_once 'kphp_tester_include.php';

class A {
    function fa() { echo "fa\n"; }
}

function fun(): callable
{
    return function (Classes\IntHolder $a) {
        return $a->a + 10;
    };
}

$ah = new Classes\IntHolder();
$res = fun()($ah);
var_dump($res);

$bh = new Classes\IntHolder();
$res = fun()($bh);
var_dump($res);

// the same, test for race condition 
function f(): callable { return fn () => 10; }

f()();
f()();
f()();
f()();
f()();
f()();
f()();
f()();
f()();


// lambdas returning lambdas
// such op_invoke_call are bound later, after all generics instantiations

function get_lambda(): callable
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
    return new Classes\CallbackHolder();
};

var_dump($f2()->get_callback()(10, 20) + 20);

// return lambda from class metho

$a = new Classes\CallbackHolder();
$fun_from_a = $a->get_callback();

var_dump($fun_from_a(10, 20));


function get_lambda_ret_A() {
    return function() { return new A; };
}
function get_lambda_ret_A_2() {
    return function(): A { $a = new A; return $a; };
}
get_lambda_ret_A()()->fa();
get_lambda_ret_A_2()()->fa();


function getCbCb($add) {
    return function() use($add) {
        return function ($x) use ($add) { return $x + $add; };
    };
}
function getCb() {
    return function($x) {
        return $x + 10;
    };
}
$res0 = array_map(getCb(), [1,2,3]);
var_dump($res0);
$res1 = array_map(getCbCb(10)(), [1,2,3]);
var_dump($res1);
$res2 = array_map(getCbCb('50')(), [1,2,3]);
var_dump($res2);
