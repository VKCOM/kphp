@ok k2_skip
<?php

/**
 * @kphp-generic T
 * @param T $t
 */
function stringify($t): string {
    return (string)$t;
}

interface StringableWith {
    /**
     * @kphp-generic T
     * @param T $with
     */
    function echoSelfWith($with);
}

class A implements StringableWith {
    /** @kphp-generic T */
    function echoSelfWith($with) {
        /** @var T */
        $with2 = $with;
        echo "A and ", stringify($with2), "\n";
    }
}

abstract class BBase implements StringableWith {
    abstract function getName(): string;

    /** @kphp-generic T */
    function echoSelfWith($with) {
        echo $this->getName(), " and ", stringify/*<T>*/($with), "\n";
    }

    function __toString(): string {
        ob_start();
        $this->echoSelfWith("__toString");
        $s = ob_get_contents();
        ob_end_clean();
        return trim($s);
    }
}

class B1 extends BBase {
    function getName(): string { return "B1"; }
}
class B2 extends BBase {
    function getName(): string { return get_class($this); }
}
class B22 extends B2 {}

function demo1() {
    $a = new A;
    $a->echoSelfWith(4);
    $b = new B1;
    $b->echoSelfWith("demo1");
    $b2 = new B2;
    $b2->echoSelfWith("demo1");
}

function demo2() {
    (new A)->echoSelfWith(new B1);
}

/**
 * @kphp-generic TWith
 * @param TWith $with
 */
function echoI(StringableWith $obj, $with) {
    $obj->echoSelfWith/*<TWith>*/($with);
}

/**
 * @kphp-generic TAny, TWith
 * @param TAny $obj
 * @param TWith $with
 */
function echoAny($obj, $with) {
    $obj->echoSelfWith/*<TWith>*/($with);
}

demo1();
demo2();
echoI(new B1, "blob");
echoI(new B22, new B2);
echoAny(new B1, "hello");
echoAny(new A, new B2);
echoAny(new A, new B22);
