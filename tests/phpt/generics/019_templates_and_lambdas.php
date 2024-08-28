@ok k2_skip
<?php
require_once 'kphp_tester_include.php';


class BB {
    function method() { echo "B method\n"; return 1; }
}
class D1 extends BB {
    function dMethod() { echo "d1\n"; }
}

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
 * @kphp-generic T
 * @param T $o
 */
function tplFWithLambda1($o) {
    $handler = function() use($o) {
        (function() use($o) {
            /** @var tuple(int, T) */
            $t = tuple(1, $o);
            $t[1]->method();
        })();
    };
    $handler();
}

tplFWithLambda1(new A);
tplFWithLambda1(new B);



/**
 * @kphp-generic T1, T2
 * @param T1 $o1
 * @param T2 $o2
 * @return ?T1
 */
function castO2ToTypeofO1($o1, $o2) {
   $h = function() use($o1, $o2) {
       /** @var T1 */
       $casted = instance_cast($o2, classof($o1));
       echo get_class($casted), "\n";
       return $casted;
   };
   return $h();
}

/** @var BB */
$bb = new D1;
castO2ToTypeofO1($bb, new D1);




interface ISignalParam  {
}

class JsonCommandParams implements ISignalParam {
}

class ApiParams1 extends JsonCommandParams {
    public int $p1 = 1;
}
class ApiParams2 extends JsonCommandParams {
    public int $p2 = 2;
}

interface ISignalResult {
    function getResult();
}
class RealSignalResult implements ISignalResult {
    function getResult() { return "RealSignalResult"; }
}

function getSignal(string $signalName): ISignalParam {
    if ($signalName === '1') return new ApiParams1;
    if ($signalName === '2') return new ApiParams2;
    return null;
}

/**
 * @param string $signalName
 * @kphp-generic T
 * @param T $params
 * @param callable(T):ISignalResult $handler
 */
function signalConnect(string $signalName, JsonCommandParams $params, callable $handler) {
    /** @var ISignalResult */
    $r = (function(ISignalParam $signalParam) use($params, $handler) {
        $param = instance_cast($signalParam, classof($params));
        return $handler($param);
    })(getSignal($signalName));
    echo $r->getResult(), "\n";
}

signalConnect('1', new ApiParams1, function(ApiParams1 $p) {
    echo $p->p1, "\n";
    return new RealSignalResult;
});
signalConnect('2', new ApiParams2, function(ApiParams2 $p) {
    echo $p->p2, "\n";
    return new RealSignalResult;
});

/**
 * @kphp-generic TIn, TOut
 * @param TIn $in
 * @param callable(TIn):TOut $cb
 * @return TOut
 */
function callAndGet($in, $cb) {
    return $cb($in);
}

callAndGet(new A, fn($a): B => new B)->method();
callAndGet(new A, function($a): ?A { return new A; })->method();


class WithData1 { public int $data = 0; function w1() {} }
class WithData2 { public ?WithData1 $data = null; }

/**
 * @kphp-generic T
 * @param T $obj
 */
function getData($obj) { return $obj->data; }

function demoWithData() {
    $ints = array_map(fn($obj) => getData($obj), [new WithData1]);
    var_dump($ints);
    /** @var (?WithData1)[] $w1 */
    $w1 = array_map(fn($obj) => getData($obj), [new WithData2]);
    if ($w1[0] === null) echo "w1 null\n";
    else $w1[0]->w1();
}

demoWithData();

