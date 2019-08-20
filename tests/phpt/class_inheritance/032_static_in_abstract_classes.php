@ok
<?php

abstract class Base {
    abstract public static function foo($x);

    public static function bar() {
        static::foo(10);
    }
}

class Derived extends Base {
    public static function foo($x) {
        var_dump($x);
    }
}

Derived::foo("asdf");
Derived::bar();

