@ok
<?php

class Base {
    /** @param callable(int):int $cb */
    function takesTypedCallback(callable $cb, int $arg) {
        echo $cb($arg), "\n";
    }

    function takesCallable(callable $cb, int $arg) {
        echo $cb($arg), "\n";
    }
}

class Derived extends Base {
    function foo() {
        parent::takesTypedCallback(fn($x) => $x, 10);
        parent::takesCallable(fn($x) => $x, 10);
        (function() {
            parent::takesTypedCallback(fn($x) => $x + 1, 10);
            parent::takesCallable(fn($x) => $x + 1, 10);
        })();
    }
    function foo2() {
        /** @var callable(int):int $cb */
        $cb = fn($x) => $x + 2;
        parent::takesTypedCallback($cb, 10);
        parent::takesCallable($cb, 10);
    }
}

$d = new Derived;
$d->foo();
$d->foo2();


class WithFSelf {
    static function f1() { echo "f1\n"; }
    static function f2() { echo "f2\n"; }

    function f() {
        $f1 = function() { self::f1(); };
        $f1();
        $f2 = fn() => self::f1();
        $f2();
    }
    static function sf() {
        $f1 = function() { static::f2(); };
        $f1();
    }
}

class WithFSelfBase {
    static function f1() { echo "f1\n"; }
    static function f2() { echo "f2\n"; }

    static function sf1() {
        $f1 = function() { self::f1(); };
        $f1();
        $f2 = fn() => self::f1();
        $f2();
    }
    static function sf2() {
        $f1 = function() { static::f2(); };
        $f1();
    }
}

class WithFSelfExtend extends WithFSelfBase {
}

(new WithFSelf)->f();
WithFSelfBase::sf1();
WithFSelfExtend::sf1();
WithFSelfBase::sf2();
WithFSelfExtend::sf2();

