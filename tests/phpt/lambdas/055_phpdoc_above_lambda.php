@ok
<?php

require_once 'kphp_tester_include.php';

class A {
    function f() { echo "A f\n"; }
    function get() { return 1; }
}

$f1 =
/**
 * @param A $a
 */
function($a) { $a->f(); };
$f1(new A);

/**
 * @param A[] $a
 */
$f2 = fn($a) => $a[0]->get();
$f2([new A]);

/**
 * @param tuple(int, A) $p1
 * @param tuple(string, A) $p2
 */
$f3 = static function($p1, $p2): int {
    $p1[1]->f();
    $p21 = $p2[1];
    $p21->f();
    return 1;
};
$f3(tuple(1, new A), tuple('', new A));

/** @return A[] */
$f4 = function() { return []; };
foreach ($f4() as $a)
    $a->f();
