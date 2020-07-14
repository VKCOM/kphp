@ok
<?php

abstract class Base {
    /** @param mixed $x */
    abstract public static function foo($x);

    public static function bar() {
        static::foo(10);
    }
}

class Derived extends Base {
    /** @param mixed $x */
    public static function foo($x) {
        var_dump($x);
    }
}

Derived::foo("asdf");
Derived::bar();

