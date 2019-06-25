@ok
<?php

class Base {
    public static function foo() {
        var_dump("Base");
    }

    public static function bar() {
        $f = function() { static::foo(); };
        $f();
    }
}

class Derived extends Base {
    public static function foo() {
        var_dump("Derived");
    }
}

Derived::bar();
