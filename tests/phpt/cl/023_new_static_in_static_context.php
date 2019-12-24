@ok
<?php

class A {
    /**@var A*/
    public static $inst;

    public function foo() {
        var_dump("a");
    }

    /** @return A */
    public static function get_inst() {
        if (!static::$inst) {
            static::$inst = new static();
        }

        return static::$inst;
    }
}

class D extends A {
    /**@var D*/
    public static $inst;

    public function foo() {
        var_dump("d");
    }
}

class C extends D {
    /**@var C*/
    public static $inst;

    public function foo() {
        var_dump("c");
    }
}

$a = A::get_inst();
$a->foo();

$d = D::get_inst();
$d->foo();

$c = C::get_inst();
$c->foo();

