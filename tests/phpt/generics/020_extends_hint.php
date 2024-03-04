@ok
<?php
require_once 'kphp_tester_include.php';

use Classes\TemplateMagic;

interface SomeI { function f(); }

class A implements SomeI { function f() { echo "A f\n"; } }
class B implements SomeI { function f() { echo "B f\n"; } }

/**
 * @kphp-generic T: \SomeI
 * @param T $o
 */
function tplOne($o) {
    if ($o !== null)
        $o->f();
}

tplOne(new A);
tplOne(new B);
tplOne/*<?B>*/(null);


/**
 * @kphp-generic T: TemplateMagic
 * @param T $o
 */
function tplTwo($o) {
    if ($o !== null)
        $o->print_magic(new Classes\A);
}

tplTwo/*<TemplateMagic>*/(null);
tplTwo(new TemplateMagic);


interface MStringable {
    function __toString(): string;
}

class S1 implements MStringable {
    function __toString(): string {
        return "S1\n";
    }
}

/**
 * @kphp-generic T: MStringable | string
 * @param T $toStr
 */
function printToString($toStr) {
    echo $toStr, "\n";
}

printToString("adsf");
printToString(new S1);


/**
 * @kphp-generic T: callable(int):void
 * @param T $cb
 */
function typedCallableGen($cb) {
    $cb(1);
}

/** @kphp-required */
function asCb($i): void {
    echo $i * 3, "\n";
}

typedCallableGen(function($i) { echo $i, "\n"; });
typedCallableGen(function($i) { echo $i*2, "\n"; });
typedCallableGen('asCb');


/**
 * @kphp-generic T1: int|string|float|null, T2: SomeI|TemplateMagic
 * @param T1 $t1
 * @param T2 $t2
 */
function strange($t1, $t2) {
    echo ($t1 === null ? 'null' : gettype($t1)), " ", ($t2 === null ? 'null' : get_class($t2)), "\n";
}

/** @var ?string $s1 */
$s1 = null;
strange($s1, new A);
strange/*<null|int, ?TemplateMagic>*/(null, null);
$s1 = 's';
strange($s1, new TemplateMagic);

