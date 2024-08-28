@ok k2_skip
<?php

function takeString(string $s) {
    return strlen($s);
}

/** @param int[] $arr */
function getFromArray(array $arr, int $idx) {
    return $arr[$idx];
}

function myArrayMap(callable $mapper, array $input) {
    $result = [];
    foreach ($input as $k => $v)
        $result[$k] = $mapper($v);
    return $result;
}


function demoSplitVar(int $u) {
    $u = 'str';
    $res1 = array_map(fn($i) => takeString($u) + $i, [1,2,3]);
    var_dump($res1);
    $res2 = myArrayMap(fn($i) => takeString($u) + $i, [1,2,3]);
    var_dump($res2);
}

function demoSmartCast() {
    /** @var int[]|false */
    $arr_false = [1,2,3];
    if ($arr_false) {
        $res1 = array_map(function($idx) use($arr_false) {
            return getFromArray($arr_false, $idx);
        }, [0,1,2]);
        var_dump($res1);
        $res2 = myArrayMap(function($idx) use($arr_false) {
            return getFromArray($arr_false, $idx);
        }, [0,1,2]);
        var_dump($res2);
    }
}


interface I {}

class A implements I {
    public function run() { echo "A run\n"; }
}

class B implements I {}

function demoWithInstanceof() {
    /**@var I*/
    $x = true ? new A : new B;
    if ($x instanceof A) {
        $fun = function() use ($x) {
            $x->run();
        };
        $fun();
    }
}


class MyEx extends Exception {
    function showMyError() { echo "showMyError\n"; }
}

function demoWithCatch() {
    try {
        if (1) throw new MyEx;
    } catch (MyEx $ex) {
        $fun = fn() => $ex->showMyError();
        $fun();
    }
}


demoSplitVar(1);
demoSmartCast();
demoWithInstanceof();
demoWithCatch();
