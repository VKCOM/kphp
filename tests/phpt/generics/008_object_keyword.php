@ok
<?php

class AMagic {
    public string $magic = 'a';
}
class BMagic {
    public string $magic = 'b';
}

function printMagic(object $arg) {
    var_dump($arg->magic);
    return $arg;
}

function demo1() {
    $b = new BMagic;
    $b->magic = '100b';
    printMagic($b);

    printMagic(new AMagic);
}
demo1();


class AFiel {
    public $fiel = 0;
}

class BFiel {
    public $fiel = 1;
}

/**
 * @kphp-template T $obj
 * @kphp-return T
 */
function same(object $obj) {
    return $obj;
}

function demoNestedTplInfer() {
    $a = new AFiel;
    $a1 = same($a);
    $a2 = same($a1);
    $a3 = same($a2);
    $a1->fiel = 9;
    echo $a->fiel;
    echo $a1->fiel;
    echo $a2->fiel;
    echo $a3->fiel;
    echo "\n";

    $b = new BFiel;
    same(same(same($b)))->fiel = 10;
    echo same(same($b))->fiel;
    echo "\n";
}

demoNestedTplInfer();

interface IStr {
    function __toString();
}
class AStr implements IStr {
    function __toString() { return __CLASS__; }
}
class BStr implements IStr {
    function __toString() { return __CLASS__; }
}

class Stringer {
    function myStr(object $i) {
        echo $i, "\n";
    }

    function myStr2(object $i) {
        $this->myStr($i);
    }
}

$ss = new Stringer;
$ss->myStr(new AStr);
$ss->myStr(new BStr);
$ss->myStr2(new AStr);
$ss->myStr2(new BStr);

/**
 * @kphp-generic T: object
 * @param T[] $objects
 */
function stringifyMany($objects) {
    foreach ($objects as $o)
        echo $o, "\n";
}

stringifyMany([new AStr]);
stringifyMany/*<IStr>*/([new BStr, new AStr]);
