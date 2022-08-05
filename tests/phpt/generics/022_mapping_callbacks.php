@ok
<?php
require_once 'kphp_tester_include.php';

class A {
    public int $a;
    function __construct(int $a = 0) { $this->a = $a; }
    function fa() { echo "A {$this->a}\n"; }
}
class B {
    public int $b;
    function __construct(int $b = 0) { $this->b = $b; }
    function fb() { echo "B {$this->b}\n"; }
}

/**
 * @kphp-generic TIn, TOut
 * @param TIn $in
 * @param callable(TIn):TOut $cb
 * @return TOut
 */
function mapOne($in, $cb) {
    /** @var TOut */
    $out = $cb($in);
    return $out;
}

mapOne(new A(-1), fn($a) => $a)->fa();
mapOne(new B(-1), fn($b) => $b)->fb();
mapOne(new B, function($_) {
    $b = new B(-2);
    $b->fb();
    $b2 = fn() => $b;
    return $b2();
})->fb();


/**
 * @kphp-generic TIn, TOut
 * @param TIn $in
 * @param callable(TIn):TOut $cb
 * @return tuple(TIn, TOut)
 */
function mapOneGetTuple($in, $cb) {
    return tuple($in, $cb($in));
}

mapOneGetTuple(new A(-10), fn($a) => $a)[0]->fa();
mapOneGetTuple(new B(-10), fn($b) => $b)[1]->fb();


/**
 * @kphp-generic TElem, TOut
 * @param TElem[] $arr
 * @param callable(TElem):TOut $callback
 * @return TOut
 */
function getFirstMapped($arr, $callback) {
    return $callback($arr[0]);
}

getFirstMapped([new A(1)], fn(A $a): A => $a)->fa();
getFirstMapped([new A(2)], fn(A $a) => $a)->fa();
getFirstMapped([new A(3)], fn($a) => $a)->fa();
getFirstMapped([new A(4)], function($a) { $a2 = $a; $a3 = $a2; $a3->fa(); return $a3; })->fa();


/**
 * @kphp-generic TElem, TOut
 * @param callable(TElem):TOut $callback
 * @param TElem[] $arr
 * @return TOut
 */
function getFirstMapped2($callback, $arr) {
    return $callback($arr[0]);
}

getFirstMapped2(fn(A $a) => new B($a->a), [new A(10)])->fb();
getFirstMapped2(fn($a) => new B($a->a), [new A(11)])->fb();
getFirstMapped2(fn($a) => $a, [new A(12)])->fa();
getFirstMapped2(function($a) { $a->fa(); $b = new B; return $b; }, [new A])->fb();

/** @var int[] */
$ints = [
    getFirstMapped2(fn($a) => 30, [new A]),
    getFirstMapped2(fn($i) => $i, [1, 2, 3]),
    getFirstMapped2(fn($i): int => $i*2, [1, 2, 3]),
    getFirstMapped2(fn($i) => getFirstMapped([$i], fn($i) => $i), [getFirstMapped2(fn($a) => $a->a, [new A(31)])]),
];
var_dump($ints);

$outer_a = new A(40);
$outer_b = new B(40);
getFirstMapped([new A(41)], fn($_) => $outer_a)->fa();
getFirstMapped([new A(42)], fn($_) => $outer_b)->fb();
getFirstMapped([new A(43)], fn($a) => new A($outer_a->a + $a->a))->fa();


/**
 * @kphp-generic TElem, TOut
 * @param callable(TElem):TOut $callback
 * @param TElem[] $arr
 * @return TOut[]
 */
function map($arr, $callback) {
    /** @var TOut[] */
    $result = [];
    foreach ($arr as $k => $v)
        $result[$k] = $callback($v);
    return $result;
}

/** @var string[] */
$s_arr = map([1,2,3], fn($i) => (string)$i);
foreach (map([100,101,102], fn($i) => new A($i + $outer_a->a)) as $a)
    $a->fa();
