@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

class A {
    function fa() { echo "fa\n"; }
    function __toString(): string { return "A"; }
}


/**
 * @param callable(int):int $cb
 */
function acceptTyped1(callable $cb) {
    echo $cb(100), "\n";
}
acceptTyped1(fn($x) => $x);
acceptTyped1(fn($x) => $x*2);
acceptTyped1(1 ? fn($x) => $x+3 : fn($x) => $x*3);

$mult = 4;
/** @var callable(int):int $cb_typed1 */
$cb_typed1 = function($x) use($mult) { return $x * $mult; };
acceptTyped1($cb_typed1);

/** @param ?callable(A):A $call */
function acceptTypedC2($call, A $arg) {
    $a = $call($arg);
    $a->fa();
}
/** @param (callable(A):A)[][] $c3_arr_arr */
function acceptDoubleCallableArr($c3_arr_arr) {
    foreach ($c3_arr_arr as $c3_arr)
        foreach ($c3_arr as $c3)
            acceptTypedC2($c3, new A);
}
function demoWithSetInside() {
    /** @var callable(A):A $c1 */
    $c1 = function($a) { $a->fa(); return $a; };
    acceptTypedC2($c1, new A);
    /** @var (callable(A):A)[] */
    $c2_arr = [
        fn($a) => $a,
        fn($a) => new A,
    ];
    foreach ($c2_arr as $c2)
        acceptTypedC2($c2, new A);
    acceptDoubleCallableArr([
        [fn($a) => $a, fn($a) => $a],
        [fn($a) => $a, fn($a) => $a],
    ]);
}
demoWithSetInside();

function demoWithSetInside2() {
    /** @var callable(A):A $c1 */
    $c1 = function($a) { $a->fa(); return $a; };
    $c1(new A);
}
demoWithSetInside2();

/** @param ?callable(int, string, int[]): bool $callable */
function acceptsUniqueAlwaysNull($callable) {
    if ($callable)
        $callable(1, 's', [1]);
}
acceptsUniqueAlwaysNull(null);


/** @var callable(): A */
$cbGetA = fn() => new A;
$cbGetA()->fa();

class HasInstanceMethod {
    public int $mult = 3;
    function myStrlenDoubling(string $s) { return strlen($s) * $this->mult; }
}
/** @param callable(string):int $s */
function canTakeTypedStrlen($s) {
    echo $s('asdf'), "\n";
}
/** @kphp-required */
function myStrlenDoubling($s) { return strlen($s) * 2; }
canTakeTypedStrlen('strlen');
canTakeTypedStrlen('myStrlenDoubling');
if (0) {
    canTakeTypedStrlen('curl_init');        // compiles also, as curl_init(string):int
}
$inst_with_cb = new HasInstanceMethod;
canTakeTypedStrlen([$inst_with_cb, 'myStrlenDoubling']);
/** @var callable */
$ccc_m = [$inst_with_cb, 'myStrlenDoubling'];
echo $ccc_m('ccc'), "\n";
var_dump(array_map($ccc_m, ['a', 'ab']));

/** @var callable(int):int */
$typed_cb_passed_to_extern = function ($x) { return $x+1; };
var_dump(array_map($typed_cb_passed_to_extern, [1,2,3]));

/**
 * @param (callable(int):int) ...$callbacks
 */
function takeVariadicCallbacks(...$callbacks) {
    foreach ($callbacks as $cb)
        echo $cb(1), "\n";
}
takeVariadicCallbacks(
    fn($x) => $x,
    fn($x) => $x + 1,
    fn($x) => $x * 2,
);

/**
 * @param tuple(callable(int):int, string, (callable(A):void)[]) $tup
 */
function takeTupleWithCallback($tup) {
    $cb1 = $tup[0];
    echo $cb1(100), "\n";
    foreach ($tup[2] as $cb2)
        $cb2(new A);
}
takeTupleWithCallback(tuple(
    fn($x) => $x+1,
    'asdf',
    [fn($a) => $a->fa(), fn($a) => $a->fa()]
));

/**
 * @param shape(cb:callable(A):A) $sh
 */
function takeShapeWithCallback($sh) {
    $cb = $sh['cb'];
    $cb(new A)->fa();
}
takeShapeWithCallback(shape([
    'cb' => function($a) { $a->fa(); return $a; },
]));

function demoWithAssignToList() {
    /**
     * @var $cbv1 callable(int):int
     * @var $cbv2 callable(int):int
     * @var $cbv3 callable(int):int
     */
    list($cbv1, $cbv2) = [fn($v) => $v+1, fn($v) => $v+2];
    list(, $cbv3) = tuple(0, fn($v) => $v+3);

    echo $cbv1(1), "\n", $cbv2(1), "\n", $cbv3(1), "\n";
}
demoWithAssignToList();

function demoSome33() {
    /** @var (callable():void)[] */
    $callbacks = [
        function() { var_dump("Hell"); },
        function() { var_dump("o "); },
    ];

    $world = "World!";
    $callbacks[] = function() use ($world) { var_dump($world); };
    $callbacks[3] = function() use ($world) { var_dump($world . '!'); };

    foreach ($callbacks as $cb)
        $cb();
}
demoSome33();

function demoSome44() {
    /** @var (?callable():string)[][]|null */
    $matrix = [];

    $matrix[] = [fn() => print_r('1')];
    $matrix[0][] = fn() => print_r('2');
    $matrix[1][0] = fn() => print_r('3');

    foreach ($matrix as $row)
        foreach ($row as $cb)
            $cb();
}
demoSome44();

function demoSome55() {
    $world = "World!";
    /**@var (callable():void)[]*/
    $callbacks = [];

    /**@var $callback callable():void*/
    $callback = function() { var_dump("Hell"); };
    $callbacks[] = $callback;

    $callback = function() { var_dump("o "); };
    $callbacks[] = $callback;

    $callback = function() use ($world) { var_dump($world); };
    $callbacks[] = $callback;

    foreach ($callbacks as $callback) {
        $callback();
    }
}
demoSome55();

function demoSome66() {
    /**@var (callable(int):int)[] $a */
    $a = [
        0 => function($x) { return $x; },
        function($y) { return 10 + $y; },
    ];

    foreach ($a as $f) {
        var_dump($f(9));
    }
}
demoSome66();

/** @var callable */
$f_strlen = 'strlen';
var_dump($f_strlen('asdf'));

/** @kphp-required */
function getAAA() { return new A; }
/** @var callable */
$f_getAAA = 'getAAA';
$aaa = $f_getAAA();
$aaa->fa();

/** @kphp-required */
function thisIsCalledAsString() { echo __FUNCTION__, "\n"; }
function getCallableReturnString(): callable { return 'thisIsCalledAsString'; }
getCallableReturnString()();

