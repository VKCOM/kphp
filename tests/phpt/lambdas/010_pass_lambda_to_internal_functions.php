@ok k2_skip
<?php

print_r(array_map(function ($n, $z = 10) {
    return $n * $n;
}, [1, 2, 3, 4, 5]));

register_shutdown_function(function () {
});

/**
 * @return string[]
 */
function h() {
    return array_filter(explode('|', "1|2|3|4|5|7"), function ($x) { return (bool)($x & 1); });
}

var_dump(h());


function g(int $x): callable { return function($a) { return $a + 10; }; }

$a = array_map(g(10), [1, 2, 3]);

var_dump($a);


$text = "asdf 1 sadf";
var_dump(preg_replace_callback("|\d|",function ($matches) { return '99'; } , $text));


/**
 * @param int[] $ints
 * @return int
 */
function get_sum($ints) {
    return (int)array_sum($ints);
}

function run22() {
    $arr = [10.123, '123.98'];
    $ints = array_map('intval', $arr);
    var_dump(get_sum($ints));
}

run22();


class HasShutdown {
    function onShut() {}
    
    function regShut() {
        if(0) register_shutdown_function([$this, 'onShut']);
        if(0) register_shutdown_function(function() {});
        $e = 10;
        if(0) register_shutdown_function(function() use($e) { echo $e; });
    }
}

(new HasShutdown)->regShut();


function run_passing_vars_to_usort(callable $fetcher, callable $comparator) {
    $res = $fetcher(10);
    usort($res, $comparator);
    var_dump($res[0], $res[1]);
}

run_passing_vars_to_usort(
    function ($x) { return [$x - 10, $x]; },
    function ($x, $y) { return $x > $y ? -1 : 1; }
);

function myFilter1(callable $cb, array $arr) {
    $out = [];
    foreach ($arr as $i) if ($cb($i)) $out[] = $i;
    return $out;
}
function myFilter2(callable $cb, array $arr) {
    return array_filter($arr, $cb);
}
$less_than_three = fn($x) => $x < 3;
var_dump(myFilter1($less_than_three, [1,2,3,4]));
var_dump(myFilter2($less_than_three, [1,2,3,4]));

function demoArrayMapWithCaptured($y1, $y2) {
    $res = array_map(fn($x) => $x + $y1 + $y2, [1]);
    var_dump($res);
}
demoArrayMapWithCaptured(1, '3');

