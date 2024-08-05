@ok k2_skip
<?php
require_once 'kphp_tester_include.php';

use Classes\A as UsedA;
use Classes\B as UsedB;

class A {
    function method() { echo "A method\n"; return 1; }
}
class B {
    function method() { echo "B method\n"; return 2; }
}
class C {
    function method() { echo "C method\n"; return 3; }
}



/**
 * @kphp-generic TElem
 * @param TElem[] $arr
 */
function acceptsArr($arr) {
    foreach ($arr as $o)
        if($o) $o->method();
}

/**
 * @kphp-generic E1
 * @param E1 $obj
 */
function callArr1($obj) {
    acceptsArr([$obj]);
}

/**
 * @kphp-generic E2
 * @param E2 $obj
 */
function callArr2($obj) {
    acceptsArr/*<E2>*/([$obj]);
}

callArr1(new A);
callArr1(new B);
callArr2(new A);
callArr2/*<B>*/(null);
callArr2(new C);



/**
 * @kphp-generic T1, T2
 * @param T1[] $arr1
 * @param T2[] $arr2
 */
function f1($arr1, $arr2) {
    foreach ($arr1 as $a1)
        if ($a1) $a1->method();
    foreach ($arr2 as $a2)
        if ($a2) $a2->method();
}

f1/*<\A, \A>*/([null], [new A]);
f1/*<B, B>*/([new B], [new B]);
f1/*<A, B>*/([new A], [new B]);


/**
 * @kphp-generic T1
 * @param T1 $first
 * @param T1[] $all
 */
function f2($first, $all) {
    if (count($all)) {
        /** @var tuple(int, T1) $f */
        $f = tuple(1, [[$all]][0][0][0]);
        if(0) $f[1]->method();
    }
    foreach ($all as $a)
        if($a) $a->method();
}

f2(new A, []);
f2(new A, [null]);


class F3c {
/**
 * @kphp-generic T
 * @param T $arg
 * @param callable(T):T $cb
 */
function f3($arg, $cb) {
    $m = $cb($arg);
    $m->method();
}
}

$f3 = new F3c;
$f3->f3(new A, fn($a) => $a);
$f3->f3(new B, function($b) { $b->method(); return $b; });

/**
 * @kphp-generic T : callable
 * @param T $cb
 */
function f4($cb) {
    echo $cb(), "\n";
}

f4(fn() => 4);
f4(fn() => 's');

/**
 * @kphp-generic T1 : callable, T2 : callable
 * @param T1 $cb1
 * @param T2 $cb2
 */
function f5($cb1, $cb2) {
    echo $cb1(), $cb2(), "\n";
}

f5(fn() => 5, fn() => 's5');


/**
 * @kphp-generic T2 : callable
 * @param T2 $cb2
 */
function f6(callable $cb1, $cb2) {
    echo $cb1(), $cb2(), "\n";
}

f6(fn() => 6, fn() => 's6');


/**
 * @kphp-generic T
 * @param tuple(int, T) $t
 */
function f7($t) {
    echo $t[0], ' ', $t[1]->method(), "\n";
}

f7/*<A>*/(tuple(1, new A));


/**
 * @kphp-generic T
 * @param T $first
 * @param T ...$rest
 */
function f8($first, ...$rest) {
    $first->method();
    foreach ($rest as $o)
        $o->method();
}

f8(new A, new A, new A);
f8(new B, new B);
f8(new C);


class F9c {
/**
 * @kphp-generic T
 * @param T[] $all
 */
static function f9($all) {
    foreach ($all as $o)
        $o->method();
}
}

F9c::f9([new A, new A, new A]);
F9c::f9([new B]);
F9c::f9/*<C>*/([]);


class F10 {
/**
 * @kphp-generic T
 * @param T ...$all
 */
function f10(...$all) {
    foreach ($all as $o)
        $o->method();
}
}

$f10 = new F10;
(function() use($f10) {
    $f10->f10(new A, new A, new A);
    $f10->f10(new B);
    $f10->f10/*<C>*/();
})();


/**
 * @kphp-generic T, TCb
 * @param T   $arg
 * @param TCb $cb
 */
function strangeCall($arg, $cb) {
    $cb($arg);
}

strangeCall/*<int, callable(int):void>*/(1, function($i) { echo $i, "\n"; });
strangeCall/*<A, callable(A):string>*/(new A, function($a) { $a->method(); return 's'; });


class AVal {
    public int $val = 0;
    function __construct(int $val) { $this->val = $val; }
    function f() { echo "AVal {$this->val}\n"; }
}

function get_int($i) { sched_yield(); return $i; }
function get_AVal($val) { sched_yield(); return new AVal($val); }
function get_tuple() { sched_yield(); return tuple(-1, new AVal(-1)); }

/** @return future<int> */
function getFuture_int($i) { return fork(get_int($i)); }
/** @return future<AVal> */
function getFuture_AVal($val) { return fork(get_AVal($val)); }
/** @return future<tuple(int, AVal)> */
function getFuture_tuple() { return fork(get_tuple()); }

/**
 * @kphp-generic T
 * @param future<T> ...$resumables
 * @return (?T)[]
 */
function wait_all(...$resumables) {
    $answers = [];
    foreach ($resumables as $key => $resumable)
        $answers[$key] = wait($resumable);
    return $answers;
}

function demoFutures() {
    $int_arr = wait_all/*<int>*/(getFuture_int(10), getFuture_int(20));
    var_dump($int_arr);

    $fut_a_arr = [getFuture_AVal(10), getFuture_AVal(20)];
    $a_arr = wait_all/*<AVal>*/(...$fut_a_arr);
    foreach ($a_arr as $a)
        $a->f();

    $tup_arr = wait_all/*<tuple(int, AVal)>*/(getFuture_tuple());
    foreach ($tup_arr as $tup)
        $tup[1]->f();
}

demoFutures();


/**
 * @kphp-generic T
 * @param T $o
 */
function fMagic($o) {
    if ($o) $o->print_magic();
    else echo "o null\n";
}

fMagic/*<UsedA>*/(null);
fMagic/*<UsedA>*/(new UsedA);
fMagic/*<UsedB>*/(null);
fMagic/*<Classes\B>*/(null);
fMagic/*<\Classes\B>*/(new \Classes\B);
