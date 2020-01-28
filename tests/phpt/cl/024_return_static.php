@ok
<?php

class A {
    /**@var A*/
    public static $inst;

    public function afoo() {
        var_dump("a");
    }

    /** @return static */
    public static function get_inst() {
        if (!static::$inst) {
            static::$inst = new static();
            return static::$inst;
        }

        return null;
        /**@var static|null*/
        $x = new static();

        return $x;
    }
}

class D extends A {
    /**@var D*/
    public static $inst;

    public function bfoo() {
        var_dump("d");
    }
}

class C extends D {
    /**@var C*/
    public static $inst;

    public function cfoo() {
        var_dump("c");
    }
}

$a = A::get_inst();
$a->afoo();

$d = D::get_inst();
$d->bfoo();

$c = C::get_inst();
$c->cfoo();


