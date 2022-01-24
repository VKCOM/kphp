@ok
<?php

require_once 'kphp_tester_include.php';

class A { public $x = 10; }

class B { public $y = 20; }

$strs_a = array_map("to_array_debug", [new A(), new A()]);
$strs_b = array_map("to_array_debug", [new B()]);
var_dump($strs_a);
var_dump($strs_b);

var_dump(array_map("is_numeric", array_map("to_array_debug", [new A()])));

var_dump(array_reduce(["123", "245", "111", "0"], "bcadd", "0"));

var_dump(array_map("array_filter", [[1, 0], [false], [2, NULL]]));

$ints = [[1, 2], [3, 4]];
$sums = array_map('intval', array_map('array_sum', $ints));
var_dump($sums);
